// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <string>
#include "vector_storage.h"
namespace megusta { class ResourceFile; namespace dts {
// TSMaterialList's serialized flag word. These names reflect the meanings
// visible in the recovered readers/renderers; the texture-loader selector
// flags are named by their observed dispatch values rather than guessed SDK
// terminology.
enum class TSMaterialFlag : std::uint32_t {
    WrapS                    = 0x00000001u,
    WrapT                    = 0x00000002u,
    Translucent              = 0x00000004u,
    NeverEnvironmentMap      = 0x00000040u,
    SelectTextureType0       = 0x00000080u,
    SelectTextureType6       = 0x00000100u,
    DynamicMaterial          = 0x10000000u,
    IFLMaterial              = 0x08000000u,
};
constexpr std::uint32_t materialFlag(TSMaterialFlag flag) {
    return static_cast<std::uint32_t>(flag);
}
constexpr bool hasMaterialFlag(std::uint32_t flags,TSMaterialFlag flag) {
    return (flags&materialFlag(flag))!=0;
}
constexpr std::uint32_t operator|(TSMaterialFlag left,TSMaterialFlag right) {
    return materialFlag(left)|materialFlag(right);
}

// Original material reader uses a shared prefix for old shape versions.
extern const char* legacyMaterialPrefix;
extern const char* materialSkinPrefix;
struct DTSMaterialTextureServices {
    void* (*loadByName)(const char* name,int textureType,bool clampToEdge)=nullptr;
    bool (*hasBitmap)(void* entry)=nullptr;
    void (*retain)(void* entry)=nullptr;
    // Returns the still-live entry, or null when the final reference destroyed it.
    void* (*release)(void* entry)=nullptr;
    std::uint32_t (*textureName)(void* entry)=nullptr;
};
void bindMaterialTextureServices(const DTSMaterialTextureServices& services);
void* loadMaterialTextureEntry(const char* name,int textureType,bool clampToEdge);
void retainMaterialTextureEntry(void* entry);
void* releaseMaterialTextureEntry(void* entry);
std::uint32_t materialTextureEntryName(void* entry);
// Pointer/count/capacity array matching the material-list vectors. Storage
// grows in the recovered 16-element blocks and remains allocated on clear.
template<class T> struct DTSArray {
    T* data=nullptr;
    std::uint32_t count=0;
    std::uint32_t capacity=0;
    std::size_t size() const { return count; }
    bool empty() const { return count==0; }
    T* begin() { return data; }
    T* end() { return count ? data+count : data; }
    const T* begin() const { return data; }
    const T* end() const { return count ? data+count : data; }
    T& operator[](std::size_t index) { return data[index]; }
    const T& operator[](std::size_t index) const { return data[index]; }
    void resize(std::uint32_t requested) {
        if (requested>capacity)
            DTS_Vector_resizeStorage(&capacity,&count,reinterpret_cast<void**>(&data),requested,sizeof(T));
        else count=requested;
    }
    void clear() { count=0; }
    void push_back(const T& value) { const auto at=count; resize(count+1); data[at]=value; }
    ~DTSArray() { if (data) std::free(data); }
    DTSArray()=default;
    DTSArray(DTSArray&& other) noexcept
        : data(other.data),count(other.count),capacity(other.capacity) {
        other.data=nullptr; other.count=other.capacity=0;
    }
    DTSArray& operator=(DTSArray&& other) noexcept {
        if (this==&other) return *this;
        if (data) std::free(data);
        data=other.data; count=other.count; capacity=other.capacity;
        other.data=nullptr; other.count=other.capacity=0;
        return *this;
    }
    DTSArray(const DTSArray&)=delete;
    DTSArray& operator=(const DTSArray&)=delete;
};
#if UINTPTR_MAX == 0xffffffffu
static_assert(sizeof(DTSArray<void*>)==12,"DTS pointer/count/capacity array width");
#endif
struct DTSMaterialTextureHandle { void* entry=nullptr; };
// The binary stores independent name and texture-handle pointer/count/capacity
// triples, followed by six independent derived metadata triples.
class DTSMaterialList {
public:
    DTSArray<char*> materialNames;
    DTSArray<DTSMaterialTextureHandle> textureHandles;
    int textureType=0;
    bool clampToEdge=false;
    virtual ~DTSMaterialList() { free(); }
    virtual void free();
    virtual void loadIndex(std::uint32_t index);
    bool loadAll();
    void unload(std::uint32_t index);
    int push_back(char* name);
    std::size_t size() const { return textureHandles.size(); }
    std::uint32_t read(ResourceFile* file);
    std::uint32_t readText(ResourceFile* file,char first);
    bool readNames(ResourceFile& file);
    bool readTextNames(ResourceFile& file,char first);
    DTSMaterialList()=default;
    DTSMaterialList(DTSMaterialList&&)=default;
    DTSMaterialList& operator=(DTSMaterialList&&)=default;
    DTSMaterialList(const DTSMaterialList&)=delete;
    DTSMaterialList& operator=(const DTSMaterialList&)=delete;
};
class TSMaterialList : public DTSMaterialList {
public:
    bool namesTransformed=false;
    // The executable stores each derived property as its own parallel array.
    // Keep these independent of the base name/texture records so allocation,
    // field order and consumers follow that organization.
    DTSArray<std::uint32_t> flags;
    DTSArray<std::int32_t> reflectanceMaps,bumpMaps,detailMaps;
    DTSArray<float> detailScales,reflectionAmounts;
    TSMaterialList()=default;
    ~TSMaterialList() override { free(); }
    TSMaterialList(TSMaterialList&&)=default;
    TSMaterialList& operator=(TSMaterialList&&)=default;
    TSMaterialList(const TSMaterialList&)=delete;
    TSMaterialList& operator=(const TSMaterialList&)=delete;
    void free() override;
    void remap(int destination,int source);
    void allocate(std::uint32_t count);
    void load(std::uint32_t index);
    void loadIndex(std::uint32_t index) override;
    void push_back(char* name,std::uint32_t flags,int reflectanceMap,int bumpMap,int detailMap,float detailScale,float reflectionAmount);
    void tsmlTransform();
    std::uint32_t read(ResourceFile* file);
    // Normalized explicit-context adapter.
    bool read(ResourceFile& file,std::uint32_t version,const char* legacyPrefix);
    std::uint32_t getFlags(int index) const;
    std::uint32_t getTextureName(int index) const;
    void setFlags(int index,std::uint32_t flags);
};
} }


