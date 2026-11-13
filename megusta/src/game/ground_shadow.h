// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
namespace megusta {
class OpenGLAdapter;
struct SceneObjectCollection;
class SceneGridIndex;
namespace dts { class TSShapeInstance; struct EngineBitmapView; struct EngineTextureEntry; }
struct PackedRowFilterTables {
    std::uint32_t previous[16],low[16],high[16],center[16];
};
void bindPackedRowFilterTables(const PackedRowFilterTables*);
void expandPackedFilteredRows(const std::uint8_t*,std::uint32_t*,int,const PackedRowFilterTables&);
extern "C" void engineBitmap_expandPackedFilteredRows_impl(const std::uint8_t*,int*,int);
void bindGroundShadowScene(SceneObjectCollection&,SceneGridIndex&);
void renderGroundShadow(const float* view,float x,float y,float z,float minX,float maxX,
    float minY,float maxY,float minZ,float maxZ,float alpha,bool disableDepthWrite);
class ShapeShadow {
public:
    ShapeShadow();
    ~ShapeShadow();
    ShapeShadow(const ShapeShadow&)=delete;
    ShapeShadow& operator=(const ShapeShadow&)=delete;
    void setProjectionDirection(const float*);
    void setShapeScale(dts::TSShapeInstance&,const float*);
    void prepareBitmap();
    void uploadMask();
    void renderShapeMask(dts::TSShapeInstance&,const float*);
    std::uint32_t textureName() const;
    dts::EngineBitmapView* bitmap=nullptr;
    dts::EngineTextureEntry* texture=nullptr;
    float projectionBasis[16],inverseProjectionBasis[16],projectionDirection[3];
    float shapeScale[3]={1,1,1},radius=1,inverseRadius=1;
    unsigned bitmapDimension;
};
struct GroundShadowCacheEntry { float parameters[4]; std::uint32_t displayList; };
struct GroundShadowCacheState {
    GroundShadowCacheEntry entries[32]{};
    GroundShadowCacheEntry* current=nullptr;
    unsigned nextEntry=0;
    OpenGLAdapter* gl=nullptr;
};
struct ShapeShadowMaskStorage {
    std::uint32_t size=0,capacity=0;
    std::uint32_t* data=nullptr;
};
extern GroundShadowCacheState groundShadowCache;
extern ShapeShadowMaskStorage shapeShadowMask;
bool lookupOrBeginGroundShadow(float,float,float,float);
void callCurrentGroundShadow();
void clearGroundShadowCache();
void initializeShapeShadowMask();
void registerShapeShadowMaskCleanup();
void releaseShapeShadowMask();
}
