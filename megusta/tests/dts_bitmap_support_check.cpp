#include "../src/dts/bitmap_support.h"
#include "../src/dts/wrappers/bitmap_support_wrappers.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace {
int paletteDestroyed=0,pngReads=0,bmpReads=0;
void destroyPalette(void* palette) { ++paletteDestroyed; ::operator delete(palette); }
bool readPng(megusta::dts::EngineBitmapView* bitmap,void*) {
    ++pngReads; bitmap->pixelFormat=3; return true;
}
bool readBmp(megusta::dts::EngineBitmapView*,void*) { ++bmpReads; return false; }
}

int main() {
    using namespace megusta::dts;
    EngineBitmapView bitmap;
    FUN_004d2d90(&bitmap);
    assert(bitmap.pixelFormat==2 && bitmap.pixels==nullptr && bitmap.palette==nullptr);
    for (unsigned i=0;i<10;++i) {
        assert(bitmap.mipOffsets[i]==0xffffffffu);
        assert(bitmap.auxiliaryMipOffsets0[i]==0xffffffffu);
        assert(bitmap.auxiliaryMipOffsets1[i]==0xffffffffu);
    }

    const std::uint8_t rgb[12]={0,20,40, 100,120,140, 200,220,240, 255,250,245};
    std::uint8_t rgbMip[3]={};
    FUN_004d3270(const_cast<std::uint8_t*>(rgb),rgbMip,2,2);
    assert(rgbMip[0]==138 && rgbMip[1]==152 && rgbMip[2]==166);
    const std::uint8_t rgba[16]={0,10,20,30, 100,110,120,130,
                                  200,210,220,230, 255,250,245,240};
    std::uint8_t rgbaMip[4]={};
    FUN_004d3390(const_cast<std::uint8_t*>(rgba),rgbaMip,2,2);
    assert(rgbaMip[0]==138 && rgbaMip[3]==157);
    const std::uint16_t packed[4]={0x0001,0x7fff,0xffff,0x0001};
    std::uint16_t packedMip=0;
    FUN_004d3080(const_cast<std::uint16_t*>(packed),&packedMip,2,2);
    assert((packedMip&1u)==1u);
    const std::uint16_t verticalPacked[2]={0x0001,0xffff};
    std::uint16_t verticalMip=0;
    FUN_004d3080(const_cast<std::uint16_t*>(verticalPacked),&verticalMip,2,1);
    assert(verticalMip==0x7bdf);

    std::uint8_t colors[6]={255,128,0,0,255,255};
    DTS_support_RGB888_to_RGB5551_impl(reinterpret_cast<std::uint16_t*>(colors),2);
    auto* converted=reinterpret_cast<std::uint16_t*>(colors);
    assert(converted[0]==0xfc01u);
    assert(converted[1]==0x07ffu);

    bitmap.pixelFormat=2;bitmap.bytesPerPixel=3;bitmap.width=2;bitmap.height=2;bitmap.mipCount=1;
    bitmap.pixels=static_cast<std::uint8_t*>(::operator new(sizeof(rgb)));
    std::memcpy(bitmap.pixels,rgb,sizeof(rgb));bitmap.pixelBytes=sizeof(rgb);
    FUN_004d34f0(&bitmap,0);
    assert(bitmap.mipCount==2 && bitmap.mipOffsets[0]==0 && bitmap.mipOffsets[1]==12);
    assert(bitmap.pixels[12]==138 && bitmap.pixels[13]==152 && bitmap.pixels[14]==166);
    FUN_004d2e90(&bitmap);

    bitmap.pixelFormat=2;bitmap.bytesPerPixel=3;bitmap.width=2;bitmap.height=2;bitmap.mipCount=1;
    bitmap.pixels=static_cast<std::uint8_t*>(::operator new(sizeof(rgb)));
    std::memcpy(bitmap.pixels,rgb,sizeof(rgb));bitmap.pixelBytes=sizeof(rgb);
    FUN_004d3730(&bitmap);
    assert(bitmap.mipCount==2 && bitmap.pixels[bitmap.mipOffsets[1]]==128);
    FUN_004d2e90(&bitmap);

    auto* palette=::operator new(8);bitmap.palette=palette;
    bindBitmapPaletteDestroy(destroyPalette);
    FUN_004d2e90(&bitmap);
    assert(paletteDestroyed==1);
    bindBitmapPaletteDestroy(nullptr);

    bindBitmapReaders(readPng,readBmp);
    auto* loaded=FUN_004d38d0(reinterpret_cast<void*>(1));
    assert(loaded && pngReads==1 && loaded->pixelFormat==3);
    FUN_004d2df0(loaded,1);
    assert(FUN_004d3920(reinterpret_cast<void*>(1))==nullptr && bmpReads==1);
    bindBitmapReaders(nullptr,nullptr);
}
