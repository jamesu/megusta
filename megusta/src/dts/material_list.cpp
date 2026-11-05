#include "material_list.h"
#include "texture_manager.h"
#include "sequence.h"
#include "../engine/resource_file.h"
#include "../engine/string_services.h"
#include <cstring>
#include <new>
namespace megusta { namespace dts {
const char* legacyMaterialPrefix="";
const char* materialSkinPrefix="skins/";
namespace {
DTSMaterialTextureServices textureServices{};
void* releaseTexture(void* entry) {
    if (!entry) return nullptr;
    if (textureServices.release) return textureServices.release(entry);
    return releaseTextureEntry(static_cast<EngineTextureEntry*>(entry));
}
char* duplicateMaterialName(const char* name) {
    const std::size_t length=name ? std::strlen(name) : 0;
    auto* copy=static_cast<char*>(::operator new(length+1));
    if (length) std::memcpy(copy,name,length);
    copy[length]=0;
    return copy;
}
void replaceMaterialName(char*& slot,const char* value) {
    char* replacement=duplicateMaterialName(value);
    ::operator delete(slot);
    slot=replacement;
}
}
void bindMaterialTextureServices(const DTSMaterialTextureServices& services) {
    textureServices=services;
}
void* loadMaterialTextureEntry(const char* name,int textureType,bool clampToEdge) {
    return textureServices.loadByName
        ? textureServices.loadByName(name,textureType,clampToEdge)
        : loadTextureByName(name,textureType,clampToEdge);
}
void retainMaterialTextureEntry(void* entry) {
    if (!entry) return;
    if (textureServices.retain) textureServices.retain(entry);
    else retainTextureEntry(static_cast<EngineTextureEntry*>(entry));
}
void* releaseMaterialTextureEntry(void* entry) { return releaseTexture(entry); }
std::uint32_t materialTextureEntryName(void* entry) {
    if (!entry) return 0;
    if (textureServices.textureName) return textureServices.textureName(entry);
    return static_cast<EngineTextureEntry*>(entry)->textureName;
}
void DTSMaterialList::free() {
    for (std::size_t index=0;index<textureHandles.size();++index) {
        ::operator delete(materialNames[index]);
        materialNames[index]=nullptr;
        textureHandles[index].entry=releaseTexture(textureHandles[index].entry);
    }
    materialNames.clear();
    textureHandles.clear();
}
void DTSMaterialList::loadIndex(std::uint32_t index) {
    if (index>=textureHandles.size()) return;
    void* entry=textureHandles[index].entry;
    if (entry) {
        const bool hasBitmap=textureServices.hasBitmap
            ? textureServices.hasBitmap(entry)
            : static_cast<EngineTextureEntry*>(entry)->bitmap!=nullptr;
        if (hasBitmap) return;
    }
    if (!materialNames[index] || !materialNames[index][0]) return;
    void* replacement=loadMaterialTextureEntry(materialNames[index],textureType,clampToEdge);
    if (replacement!=entry) {
        releaseTexture(entry);
        textureHandles[index].entry=replacement;
        retainMaterialTextureEntry(replacement);
    }
}
bool DTSMaterialList::loadAll() {
    std::uint32_t index=0;
    while (index<textureHandles.size()) {
        // The executable dispatches this operation through the material-list
        // vtable and rereads the current list count after each callback.
        loadIndex(index);
        ++index;
    }
    return true;
}
void DTSMaterialList::unload(std::uint32_t index) {
    if (index<textureHandles.size())
        textureHandles[index].entry=releaseTexture(textureHandles[index].entry);
}
void TSMaterialList::free() {
    for (std::size_t index=0;index<textureHandles.size();++index)
        if (index<flags.size() && hasMaterialFlag(flags[index],TSMaterialFlag::IFLMaterial) &&
            index<materialNames.size())
            materialNames[index]=nullptr;
    flags.clear(); reflectanceMaps.clear(); bumpMaps.clear(); detailMaps.clear();
    detailScales.clear(); reflectionAmounts.clear();
    DTSMaterialList::free();
}
void TSMaterialList::remap(int destination,int source) {
    const auto target=static_cast<std::size_t>(destination),origin=static_cast<std::size_t>(source);
    textureHandles[target].entry=releaseTexture(textureHandles[target].entry);
    textureHandles[target].entry=textureHandles[origin].entry;
    if (textureHandles[target].entry && textureServices.retain) textureServices.retain(textureHandles[target].entry);
    // 004ce740 aliases the source name pointer as well as the texture entry.
    materialNames[target]=materialNames[origin];
}
void TSMaterialList::allocate(std::uint32_t count) {
    flags.resize(count);
    reflectanceMaps.resize(count);
    bumpMaps.resize(count);
    detailMaps.resize(count);
    detailScales.resize(count);
    reflectionAmounts.resize(count);
}
void TSMaterialList::load(std::uint32_t index) {
    // The recovered TSMaterialList_load body indexes flags before calling the
    // guarded base texture loader. Preserve that access order for invalid
    // indices instead of adding a wrapper-level bounds check.
    const std::uint32_t materialFlags=flags[index];
    textureType=hasMaterialFlag(materialFlags,TSMaterialFlag::SelectTextureType0) ? 0 :
        (hasMaterialFlag(materialFlags,TSMaterialFlag::SelectTextureType6) ? 6 : 3);
    DTSMaterialList::loadIndex(index);
}
void TSMaterialList::loadIndex(std::uint32_t index) {
    load(index);
}
int DTSMaterialList::push_back(char* name) {
    const int index=static_cast<int>(textureHandles.size());
    materialNames.push_back(duplicateMaterialName(name));
    textureHandles.push_back({nullptr});
    return index;
}
void TSMaterialList::push_back(char* name,std::uint32_t flags,int reflectanceMap,int bumpMap,int detailMap,float detailScale,float reflectionAmount) {
    int index=DTSMaterialList::push_back(name);
    this->flags.push_back(flags);
    reflectanceMaps.push_back(reflectanceMap==-1 ? index : reflectanceMap);
    bumpMaps.push_back(bumpMap); detailMaps.push_back(detailMap);
    detailScales.push_back(detailScale); reflectionAmounts.push_back(reflectionAmount);
}
void TSMaterialList::tsmlTransform() {
    if (namesTransformed) return;
    namesTransformed=true;
    for (std::size_t i=0;i<materialNames.size();++i) {
        if (!materialNames[i]) continue;
        std::string name=materialNames[i];
        for (char& c:name) if (c=='\\') c='/';
        auto dot=name.find_last_of('.');
        if (dot!=std::string::npos && (!strings::compareInsensitive(name.c_str()+dot,".bmp") ||
            !strings::compareInsensitive(name.c_str()+dot,".png"))) name.resize(dot);
        const std::string transformed=std::string(materialSkinPrefix)+name;
        replaceMaterialName(materialNames[i],transformed.c_str());
    }
}
std::uint32_t TSMaterialList::read(ResourceFile* file) {
    return read(*file,currentShapeReadVersion,legacyMaterialPrefix) ? 1u : 0u;
}
std::uint32_t DTSMaterialList::read(ResourceFile* file) { return readNames(*file) ? 1u : 0u; }
std::uint32_t DTSMaterialList::readText(ResourceFile* file,char first) { return readTextNames(*file,first) ? 1u : 0u; }
bool DTSMaterialList::readTextNames(ResourceFile& file,char first) {
    free();
    constexpr std::size_t materialTextLineCapacity=1023;
    // The original treats an empty initial byte as failure only for clean EOF;
    // a file-error state takes the other return branch and yields success.
    if (!first) return file.error() || !file.eof();
    std::string current(1,first);
    current.reserve(materialTextLineCapacity);
    for (;;) {
        bool lineReadEnded=false;
        char c=0;
        while (current.size()<materialTextLineCapacity) {
            if (file.read(&c,1,1)!=1) { lineReadEnded=true; break; }
            if (c=='\n') { lineReadEnded=true; break; }
            if (c!='\r') current.push_back(c);
        }
        if (current.empty())
            return file.error() || !file.eof();
        push_back(&current[0]);
        current.clear();
        // The original fixed stack buffer stops consuming input at its final
        // byte. If a line exceeds 1023 characters, the next outer iteration
        // treats the unread tail as the next material name.
        if (!lineReadEnded) continue;
        // Even after EOF with a nonempty line, the original loops once more;
        // the empty-line exit then returns false only for clean EOF.
    }
}
bool DTSMaterialList::readNames(ResourceFile& file) {
    free(); char marker=0;
    const bool markerRead=file.read(&marker,1,1)==1;
    // The saved reader dispatches to text only when the one-byte read
    // succeeded and the marker is not 1. On a failed marker read it still
    // attempts the following 32-bit count read.
    if (markerRead && marker!=1) return readTextNames(file,marker);
    std::uint32_t count=0; if (file.read(&count,4,1)!=1) return false;
    // The saved base reader does not branch on the per-name fread return
    // values. Its 256-byte stack buffer is reused across records; initialize
    // it once here to give the original's otherwise indeterminate first
    // short read a deterministic normalized representation.
    std::uint8_t length=0;
    char name[256]={};
    for (std::uint32_t i=0;i<count;++i) {
        (void)file.read(&length,1,1);
        (void)file.read(name,1,length);
        name[length]=0;
        if (!name[0]) return false;
        push_back(name);
    }
    return !file.error();
}
bool TSMaterialList::read(ResourceFile& file,std::uint32_t version,const char* legacyPrefix) {
    if (!readNames(file)) return false;
    // 004cea50 issues the complete metadata read loops and consults the
    // stream error flag only after applying all version defaults and name
    // changes. Do not turn a short field read into an early return here.
    auto word=[&](void* p) { (void)file.read(p,4,1); };
    allocate(static_cast<std::uint32_t>(size()));
    // Keep truncated resource reads deterministic after replacing the old
    // value-initializing std::vectors with the original raw arrays.
    for (std::size_t i=0;i<size();++i) {
        flags[i]=0; reflectanceMaps[i]=0; bumpMaps[i]=0; detailMaps[i]=0;
        detailScales[i]=0.0f; reflectionAmounts[i]=0.0f;
    }
    for (std::size_t i=0;i<size();++i) { if (version<2) flags[i]=3; else word(&flags[i]); }
    if (version<5) {
        for (std::size_t i=0;i<size();++i) reflectanceMaps[i]=static_cast<int>(i);
    } else {
        for (auto& value:reflectanceMaps) word(&value);
        for (auto& value:bumpMaps) word(&value);
        for (auto& value:detailMaps) word(&value);
    }
    if (version<5) {
        for (auto& value:bumpMaps) value=-1;
        for (auto& value:detailMaps) value=-1;
    }
    if (version<12) for (auto& value:detailScales) value=1.0f;
    if (version<21) for (auto& value:reflectionAmounts) value=1.0f;
    if (version>=12) for (auto& value:detailScales) word(&value);
    if (version>=21) for (auto& value:reflectionAmounts) word(&value);
    for (std::size_t i=0;i<size();++i) {
        if (!materialNames[i]) continue;
        std::string name=materialNames[i];
        if (version<16) {
            if (hasMaterialFlag(flags[i],TSMaterialFlag::Translucent))
                flags[i]|=materialFlag(TSMaterialFlag::NeverEnvironmentMap);
            if (legacyPrefix && *legacyPrefix) name=std::string(legacyPrefix)+name;
        }
        auto dot=name.find_last_of('.');
        if (hasMaterialFlag(flags[i],TSMaterialFlag::IFLMaterial) ||
            (version<6 && dot!=std::string::npos && !strings::compareInsensitive(name.c_str()+dot,".ifl"))) {
            ::operator delete(materialNames[i]);
            materialNames[i]=nullptr;
        } else if (name!=materialNames[i]) {
            replaceMaterialName(materialNames[i],name.c_str());
        }
    }
    return !file.error();
}
std::uint32_t TSMaterialList::getFlags(int index) const { return flags[index]; }
std::uint32_t TSMaterialList::getTextureName(int index) const {
    const auto entry=textureHandles[static_cast<std::size_t>(index)].entry;
    return materialTextureEntryName(entry);
}
void TSMaterialList::setFlags(int index,std::uint32_t value) { flags[index]=value; }
} }


