#pragma once
#include <cstdint>
#include <memory>
namespace megusta { class ResourceMountTable; }
namespace megusta::pix {
struct MultiTextureList {
    std::uint32_t textureCount=0;
    std::unique_ptr<std::uint32_t[]> textureNames;
};
MultiTextureList* loadMultiTexture(ResourceMountTable&,const char*,bool external,
    int mipmaps,int wrap,int minFilter,int magFilter);
std::uint32_t getMultiTexture(const MultiTextureList*,int);
void destroyMultiTexture(MultiTextureList*);
std::uint32_t bindPngTexture(ResourceMountTable&,const char*,int mipmaps,int wrap,int minFilter,int magFilter,int* width=nullptr,int* height=nullptr);
std::uint32_t bindPngTexture(const char*,int mipmaps,int wrap,int minFilter,int magFilter);
bool loadBoundTexture(const char*,int* width,int* height,int* alpha);
}
