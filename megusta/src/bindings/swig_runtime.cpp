#include "swig_runtime.h"
#include "../game/recovered_binding_primitives.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace megusta::bindings {
namespace {
PyTypeObject swigVarLinkType{};
bool swigVarLinkTypeReady=false;
SwigTypeMapping* swigTypeMappings=nullptr;
int swigTypeMappingCount=0;
int swigTypeMappingCapacity=32;
bool swigTypeMappingSorted=false;
int swigTypeInitialIndex[256]{};
struct PointerCacheEntry {
    int hits;
    SwigTypeMapping* mapping;
    char expectedType[256];
    char actualType[256];
};
PointerCacheEntry swigPointerCache[8]{};
int swigPointerCacheReplace=0;
unsigned swigPointerCacheScan=0;
SwigVarLink** swigVarLinkOwners[4]{};

void clearVarLinkOwner(SwigVarLink* self) {
    for (auto*& owner:swigVarLinkOwners) {
        if (owner && *owner==self) { *owner=nullptr; owner=nullptr; }
    }
}

void initializeVarLinkType() {
    if (swigVarLinkTypeReady) return;
    swigVarLinkType.ob_refcnt=1;
    swigVarLinkType.ob_type=&PyType_Type;
    swigVarLinkType.tp_name=const_cast<char*>("swigvarlink");
    swigVarLinkType.tp_basicsize=sizeof(SwigVarLink);
    swigVarLinkType.tp_dealloc=swig_varlink_dealloc;
    swigVarLinkType.tp_print=swig_varlink_print;
    swigVarLinkType.tp_getattr=swig_varlink_getattr;
    swigVarLinkType.tp_setattr=swig_varlink_setattr;
    swigVarLinkType.tp_repr=swig_varlink_repr;
    swigVarLinkTypeReady=true;
}

bool copyTypeName(char destination[256],const char* source) {
    const std::size_t length=std::strlen(source);
    if (length>=256) return false;
    std::memcpy(destination,source,length+1);
    return true;
}
}

SwigVarLink* swig_varlink_new() {
    initializeVarLinkType();
    auto* self=static_cast<SwigVarLink*>(std::malloc(sizeof(SwigVarLink)));
    if (!self) { PyErr_NoMemory(); return nullptr; }
    self->ob_refcnt=1;
    self->ob_type=&swigVarLinkType;
    self->variables=static_cast<SwigVariable**>(std::malloc(64*sizeof(SwigVariable*))); 
    if (!self->variables) { std::free(self); PyErr_NoMemory(); return nullptr; }
    self->count=0;
    self->capacity=64;
    self->variables[0]=nullptr;
    return self;
}

PyObject* swig_varlink_repr(PyObject*) {
    return PyString_FromString("<Global variables>");
}
int swig_varlink_print(PyObject* object,FILE* stream,int) {
    auto* self=reinterpret_cast<SwigVarLink*>(object);
    std::fprintf(stream,"Global variables { ");
    for (int i=0;i<self->count;++i) {
        if (i) std::fprintf(stream,", ");
        std::fprintf(stream,"%s",self->variables[i]->name);
    }
    std::fprintf(stream," }");
    return 0;
}
PyObject* swig_varlink_getattr(PyObject* object,char* name) {
    auto* self=reinterpret_cast<SwigVarLink*>(object);
    for (int i=0;i<self->count;++i) {
        SwigVariable* variable=self->variables[i];
        if (std::strcmp(variable->name,name)==0) return variable->getter();
    }
    char message[128];
    std::sprintf(message,"C global variable %s not found.",name);
    PyErr_SetString(PyExc_NameError,message);
    return nullptr;
}
int swig_varlink_setattr(PyObject* object,char* name,PyObject* value) {
    auto* self=reinterpret_cast<SwigVarLink*>(object);
    for (int i=0;i<self->count;++i) {
        SwigVariable* variable=self->variables[i];
        if (std::strcmp(variable->name,name)==0) return variable->setter(value);
    }
    char message[128];
    std::sprintf(message,"C global variable %s not found.",name);
    PyErr_SetString(PyExc_NameError,message);
    return 1;
}

void swig_varlink_add(SwigVarLink* self,const char* name,SwigGetter getter,SwigSetter setter) {
    if (self->capacity-1<=self->count) {
        const int oldCapacity=self->capacity;
        const int newCapacity=oldCapacity*2;
        auto** variables=static_cast<SwigVariable**>(
            std::realloc(self->variables,static_cast<std::size_t>(newCapacity)*sizeof(SwigVariable*)));
        if (!variables) {
            std::fprintf(stderr,"SWIG: Fatal error in initializing variable link.\n");
            std::exit(1);
        }
        self->variables=variables;
        self->capacity=newCapacity;
    }
    auto* variable=static_cast<SwigVariable*>(std::malloc(sizeof(SwigVariable)));
    if (!variable) { PyErr_NoMemory(); return; }
    const std::size_t length=std::strlen(name);
    variable->name=static_cast<char*>(std::malloc(length+1));
    if (!variable->name) { std::free(variable); PyErr_NoMemory(); return; }
    std::memcpy(variable->name,name,length+1);
    variable->getter=getter;
    variable->setter=setter;
    self->variables[self->count++]=variable;
    self->variables[self->count]=nullptr;
}

void swig_varlink_destroy(SwigVarLink* self) {
    if (!self) return;
    clearVarLinkOwner(self);
    if (self->variables) {
        for (int i=0;i<self->count;++i) {
            SwigVariable* variable=self->variables[i];
            if (!variable) continue;
            std::free(variable->name);
            std::free(variable);
            self->variables[i]=nullptr;
        }
        std::free(self->variables);
    }
    self->capacity=0;
    self->count=0;
    self->variables=nullptr;
    std::free(self);
}
void swig_varlink_dealloc(PyObject* object) {
    swig_varlink_destroy(reinterpret_cast<SwigVarLink*>(object));
}

void swig_track_varlink(SwigVarLink** owner,SwigVarLink* link) {
    if (!owner) return;
    *owner=link;
    for (auto*& slot:swigVarLinkOwners) if (!slot || slot==owner) { slot=owner; return; }
}

void swig_register_type_mapping(const char* original,const char* equivalent,SwigCast cast) {
    if (!swigTypeMappings) {
        swigTypeMappings=static_cast<SwigTypeMapping*>(
            std::malloc(static_cast<std::size_t>(swigTypeMappingCapacity)*sizeof(SwigTypeMapping)));
        if (!swigTypeMappings) { PyErr_NoMemory(); return; }
        swigTypeMappingCount=0;
    }
    if (swigTypeMappingCapacity<=swigTypeMappingCount) {
        const int oldCapacity=swigTypeMappingCapacity;
        auto* mappings=static_cast<SwigTypeMapping*>(std::realloc(
            swigTypeMappings,static_cast<std::size_t>(oldCapacity*2)*sizeof(SwigTypeMapping)));
        if (!mappings) { PyErr_NoMemory(); return; }
        swigTypeMappings=mappings;
        swigTypeMappingCapacity=oldCapacity*2;
    }
    SwigTypeMapping* root=nullptr;
    for (int i=0;i<swigTypeMappingCount;++i) {
        if (std::strcmp(swigTypeMappings[i].name,original)==0) { root=&swigTypeMappings[i]; break; }
    }
    if (!root) {
        root=&swigTypeMappings[swigTypeMappingCount++];
        root->name=const_cast<char*>(original);
        root->nameLength=static_cast<int>(std::strlen(original));
        root->cast=nullptr;
        root->next=nullptr;
    }
    SwigTypeMapping* current=root;
    while (current->next) {
        if (std::strcmp(current->next->name,equivalent)==0) {
            if (cast) current->next->cast=cast;
            return;
        }
        current=current->next;
    }
    auto* mapping=static_cast<SwigTypeMapping*>(std::malloc(sizeof(SwigTypeMapping)));
    if (!mapping) { PyErr_NoMemory(); return; }
    mapping->name=const_cast<char*>(equivalent);
    mapping->nameLength=static_cast<int>(std::strlen(equivalent));
    mapping->cast=cast;
    mapping->next=nullptr;
    current->next=mapping;
    swigTypeMappingSorted=false;
}

void swig_encode_pointer(char* buffer,const void* address,const char* typeName) {
    if (!address) { binding_string_copy(buffer,"NULL"); return; }
    static constexpr char digits[]="0123456789abcdef";
    char reverse[sizeof(std::uintptr_t)*2];
    std::uintptr_t value=reinterpret_cast<std::uintptr_t>(address);
    std::size_t length=0;
    do { reverse[length++]=digits[value&0xf]; value>>=4; } while (value);
    *buffer++='_';
    while (length) *buffer++=reverse[--length];
    *buffer=0;
    binding_string_copy(buffer,typeName);
}

char* swig_decode_pointer(const char* encoded,void** addressOut,const char* expectedType) {
    std::uintptr_t address=0;
    if (*encoded!='_') {
        if (std::strcmp(encoded,"NULL")==0) { *addressOut=nullptr; return nullptr; }
        *addressOut=nullptr;
        return const_cast<char*>(encoded);
    }
    const char* actualType=encoded+1;
    const char* cursor=actualType;
    while ((*cursor>='0'&&*cursor<='9')||(*cursor>='a'&&*cursor<='f')) {
        const unsigned digit=*cursor<='9'?static_cast<unsigned>(*cursor-'0'):
            static_cast<unsigned>(*cursor-'a'+10);
        address=(address<<4)|digit;
        ++cursor;
    }
    actualType=cursor;
    if (!expectedType) { *addressOut=reinterpret_cast<void*>(address); return nullptr; }
    if (std::strcmp(expectedType,actualType)==0) {
        *addressOut=reinterpret_cast<void*>(address);
        return nullptr;
    }
    if (!swigTypeMappingSorted) {
        std::qsort(swigTypeMappings,static_cast<std::size_t>(swigTypeMappingCount),
            sizeof(SwigTypeMapping),swig_compare_mapping_names);
        for (int& index:swigTypeInitialIndex) index=swigTypeMappingCount;
        for (int i=swigTypeMappingCount-1;i>=0;--i) {
            const auto key=static_cast<unsigned char>(swigTypeMappings[i].name[1]);
            swigTypeInitialIndex[key]=i;
        }
        for (int i=255;i>0;--i)
            if (swigTypeInitialIndex[i]<swigTypeInitialIndex[i-1])
                swigTypeInitialIndex[i-1]=swigTypeInitialIndex[i];
        swigTypeMappingSorted=true;
        for (auto& entry:swigPointerCache) entry.hits=0;
    }
    for (int attempt=0;attempt<8;++attempt) {
        const unsigned index=swigPointerCacheScan&7;
        swigPointerCacheScan=(swigPointerCacheScan+1)&7;
        auto& entry=swigPointerCache[index];
        if (entry.hits && std::strcmp(entry.expectedType,expectedType)==0 &&
            std::strcmp(entry.actualType,actualType)==0) {
            ++entry.hits;
            *addressOut=reinterpret_cast<void*>(address);
            if (entry.mapping->cast) *addressOut=entry.mapping->cast(*addressOut);
            return nullptr;
        }
    }
    const unsigned first=static_cast<unsigned char>(expectedType[1]);
    const int begin=swigTypeInitialIndex[first];
    const int end=first==255?swigTypeMappingCount:swigTypeInitialIndex[first+1];
    int index=begin;
    while (index<end && swig_compare_type_prefix(expectedType,&swigTypeMappings[index])!=0) ++index;
    if (index==end) { *addressOut=reinterpret_cast<void*>(address); return const_cast<char*>(actualType); }
    SwigTypeMapping* root=&swigTypeMappings[index];
    for (;root && swig_compare_type_prefix(expectedType,root)==0;root++) {
        for (SwigTypeMapping* mapping=root->next;mapping;mapping=mapping->next) {
            if (mapping->nameLength>254) { *addressOut=reinterpret_cast<void*>(address); return const_cast<char*>(actualType); }
            char combined[256];
            const std::size_t equivalentLength=std::strlen(mapping->name);
            if (equivalentLength+std::strlen(expectedType+root->nameLength)>=sizeof(combined))
                continue;
            std::memcpy(combined,mapping->name,equivalentLength);
            std::strcpy(combined+equivalentLength,expectedType+root->nameLength);
            if (std::strcmp(actualType,combined)!=0) continue;
            auto& cache=swigPointerCache[swigPointerCacheReplace&7];
            if (!copyTypeName(cache.actualType,actualType)||!copyTypeName(cache.expectedType,expectedType))
                break;
            cache.mapping=mapping;
            cache.hits=1;
            // The original writes back the masked replacement slot; keep the
            // observed index behavior rather than inventing an LRU policy.
            swigPointerCacheReplace&=7;
            *addressOut=reinterpret_cast<void*>(address);
            if (mapping->cast) *addressOut=mapping->cast(*addressOut);
            return nullptr;
        }
    }
    *addressOut=reinterpret_cast<void*>(address);
    return const_cast<char*>(actualType);
}

int swig_compare_mapping_names(const void* left,const void* right) {
    auto* a=static_cast<const SwigTypeMapping*>(left);
    auto* b=static_cast<const SwigTypeMapping*>(right);
    return std::strcmp(a->name,b->name);
}
int swig_compare_type_prefix(const char* name,const SwigTypeMapping* mapping) {
    return std::strncmp(name,mapping->name,static_cast<std::size_t>(mapping->nameLength));
}
void* swig_identity_upcast(void* object) { return object; }
}
