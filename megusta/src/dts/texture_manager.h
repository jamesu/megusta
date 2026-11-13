// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstddef>
#include <cstdint>
#include "vector_storage.h"

namespace megusta { namespace dts {

struct EngineBitmapView;
struct EngineTextureEntry {
    EngineTextureEntry* next=nullptr;
    EngineTextureEntry* previous=nullptr;
    EngineTextureEntry* hashNext=nullptr;
    const char* filename=nullptr;
    std::uint32_t textureName=0;
    std::uint32_t alternateTextureName=0;
    std::int32_t referenceCount=0;
    std::int32_t kind=0;
    EngineBitmapView* bitmap=nullptr;
    std::uint32_t bitmapWidth=0,bitmapHeight=0;
    std::uint32_t textureWidth=0,textureHeight=0;
    std::uint32_t downloadWidth=0,downloadHeight=0;
    std::uint8_t clamp=0;
};
struct EngineTextureHandle { EngineTextureEntry* entry=nullptr; };
#if UINTPTR_MAX == 0xffffffffu
static_assert(offsetof(EngineTextureEntry,textureName)==0x10,"texture name field");
static_assert(offsetof(EngineTextureEntry,bitmap)==0x20,"texture bitmap field");
static_assert(offsetof(EngineTextureEntry,bitmapWidth)==0x24,"texture bitmap dimensions");
static_assert(offsetof(EngineTextureEntry,clamp)==0x3c,"texture clamp field");
static_assert(sizeof(EngineTextureEntry)==0x40,"recovered 32-bit texture entry header");
#endif
using TextureEntryDestroy = void (*)(EngineTextureEntry*);
using TextureFatalExit = void (*)();
using TextureBitmapLoader = EngineBitmapView* (*)(const char* path);
extern EngineTextureEntry* textureEntryList;
extern EngineTextureEntry** textureEntryBuckets;
extern std::uint32_t textureEntryBucketCount;
void bindTextureEntryDestroy(TextureEntryDestroy destroy);
void bindTextureFatalExit(TextureFatalExit fatalExit);
void bindTextureBitmapLoader(TextureBitmapLoader loader);
void destroyTextureEntryList();
void unlinkTextureEntry(EngineTextureEntry* entry);
void linkTextureEntry(EngineTextureEntry* entry);
std::uint32_t textureFilenameHash(const std::uint8_t* filename);
void initializeTextureEntryList();
std::uint32_t uploadBitmapMipChain(EngineBitmapView* bitmap,bool clamp,std::uint32_t firstMip=0);
EngineTextureEntry* createDynamicTextureEntry(const char* filename,EngineBitmapView* bitmap,
                                              bool clamp);
void destroyTextureEntry(EngineTextureEntry* entry);
EngineTextureEntry* findTextureEntry(const char* filename,int kind);
void updateTexturePixels(EngineTextureEntry* entry);
EngineTextureEntry* loadTextureByName(const char* filename,int kind,bool clamp);
EngineTextureEntry* createOrReplaceTextureEntry(const char* filename,
    EngineBitmapView* bitmap,int kind,bool clamp);
void retainTextureEntry(EngineTextureEntry* entry);
EngineTextureEntry* releaseTextureEntry(EngineTextureEntry* entry);
bool mapTexturePixelFormat(const EngineBitmapView* bitmap,int* format,int* internalFormat,
                           int* pixelType);

} }

extern "C" {
void FUN_004cd9f0(void);
void FUN_004cda90(megusta::dts::EngineTextureEntry* entry);
void FUN_004cdb00(megusta::dts::EngineTextureEntry* entry);
void FUN_004cdb60(void);
int FUN_004ce4e0(const std::uint8_t* filename);
}
