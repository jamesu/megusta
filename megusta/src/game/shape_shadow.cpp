#include "ground_shadow.h"
#include "scene_grid.h"
#include "engine/engine_math.h"
#include "engine/opengl_adapter.h"
#include "dts/shape_instance.h"
#include "dts/bitmap_support.h"
#include "dts/texture_manager.h"
#include "dts/matrix.h"
#include <GL/gl.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace megusta {
namespace {
int shadowInstances=0;
constexpr PackedRowFilterTables recoveredPackedFilterTables={
    {0,0,0,0,0,0,0,0,17,17,17,17,17,17,17,17},
    {0,0x00001122,0x00112211,0x00113333,0x11221100,0x11222222,0x11333311,0x11334433,
     0x22110000,0x22111122,0x22222211,0x22223333,0x33331100,0x33332222,0x33443311,0x33444433},
    {0,0x11000000,0,0x11000000,0,0x11000000,0,0x11000000,
     0,0x11000000,0,0x11000000,0,0x11000000,0,0x11000000},
    {0,0x00002233,0x00223322,0x00225555,0x22332200,0x22334433,0x22555522,0x22557755,
     0x33220000,0x33222233,0x33443322,0x33445555,0x55552200,0x55554433,0x55775522,0x55777755}
};
const PackedRowFilterTables* packedFilterTables=&recoveredPackedFilterTables;
SceneObjectCollection* shadowObjects=nullptr;
SceneGridIndex* shadowGrid=nullptr;
}
void bindPackedRowFilterTables(const PackedRowFilterTables* tables) {
    packedFilterTables=tables ? tables : &recoveredPackedFilterTables;
}
void bindGroundShadowScene(SceneObjectCollection& objects,SceneGridIndex& grid) { shadowObjects=&objects; shadowGrid=&grid; }
ShapeShadow::ShapeShadow() { ++shadowInstances; }
ShapeShadow::~ShapeShadow() {
    if (!--shadowInstances) {
        void* storage=shapeShadowMask.data;
        dts::DTS_Vector_resizeStorage(&shapeShadowMask.capacity,&shapeShadowMask.size,&storage,0,4);
        shapeShadowMask.data=static_cast<std::uint32_t*>(storage);
    }
    dts::releaseTextureEntry(texture);
}
void ShapeShadow::setProjectionDirection(const float* direction) {
    std::memcpy(projectionDirection,direction,sizeof(projectionDirection));
    float first[3],last[3]; const float x=direction[0],y=direction[1],z=direction[2];
    if (std::fabs(z)<=0.001L) {
        first[0]=y; first[1]=-x; first[2]=0; engine_math::engineMath_normalizeVec3Unchecked_impl(first);
        last[0]=first[1]*z-first[2]*y; last[1]=first[2]*x-first[0]*z; last[2]=first[0]*y-first[1]*x;
    } else {
        last[0]=0; last[1]=z; last[2]=-y; engine_math::engineMath_normalizeVec3Unchecked_impl(last);
        first[0]=last[2]*y-last[1]*z; first[1]=last[0]*z-last[2]*x; first[2]=last[1]*x-last[0]*y;
    }
    engine_math::engineMatrix_identity_impl(projectionBasis);
    dts::setMatrixColumn3(projectionBasis,0,first); dts::setMatrixColumn3(projectionBasis,1,direction); dts::setMatrixColumn3(projectionBasis,2,last);
}
void ShapeShadow::setShapeScale(dts::TSShapeInstance& instance,const float* scale) {
    const auto& bounds=instance.shape->bounds;
    const float x=(bounds[3]-bounds[0])*scale[0]*0.5f,y=(bounds[4]-bounds[1])*scale[1]*0.5f,z=(bounds[5]-bounds[2])*scale[2]*0.5f;
    const float squared=x*x+y*y+z*z; const long double length=std::sqrt(static_cast<long double>(squared));
    radius=static_cast<float>(length); inverseRadius=static_cast<float>(1.0L/length); std::memcpy(shapeScale,scale,sizeof(shapeScale));
}
void ShapeShadow::prepareBitmap() {
    if (!bitmap) {
        bitmapDimension=64; bitmap=new dts::EngineBitmapView;
        dts::constructEngineBitmap(bitmap,bitmapDimension,bitmapDimension,false,1);
        auto* entry=dts::createDynamicTextureEntry(nullptr,bitmap,false);
        if (entry!=texture) { dts::releaseTextureEntry(texture); texture=entry; dts::retainTextureEntry(texture); }
    }
    const unsigned words=(bitmapDimension>>5)*bitmapDimension;
    void* storage=shapeShadowMask.data;
    if (shapeShadowMask.capacity<words) dts::DTS_Vector_resizeStorage(&shapeShadowMask.capacity,&shapeShadowMask.size,&storage,words,4);
    shapeShadowMask.data=static_cast<std::uint32_t*>(storage); shapeShadowMask.size=words;
    std::memset(storage,0,(bitmapDimension>>3)*bitmapDimension);
}
void ShapeShadow::renderShapeMask(dts::TSShapeInstance& instance,const float* transform) {
    std::memcpy(inverseProjectionBasis,projectionBasis,sizeof(projectionBasis)); engine_math::engineMatrix_inverseAffine_impl(inverseProjectionBasis);
    float matrix[16]; engine_math::engineMatrix_multiply4x4_impl(inverseProjectionBasis,transform,matrix);
    const float factor=bitmapDimension/(radius+radius);
    const float scale[3]={static_cast<float>(factor*shapeScale[0]),static_cast<float>(factor*shapeScale[1]),static_cast<float>(factor*shapeScale[2])};
    engine_math::engineMatrix_scaleRows3_impl(matrix,scale);
    float center[3]; engine_math::engineMatrix_transformVector3_impl(matrix,instance.shape->center,center);
    const float half=static_cast<float>(bitmapDimension>>1);
    const float translation[3]={static_cast<float>(half-center[0]),static_cast<float>(-center[1]),static_cast<float>(half-center[2])};
    dts::setMatrixColumn3(matrix,3,translation);
    instance.renderShadow(instance.getCurrentDetail(),matrix,bitmapDimension,shapeShadowMask.data);
}
void ShapeShadow::uploadMask() {
    expandPackedFilteredRows(reinterpret_cast<const std::uint8_t*>(shapeShadowMask.data),
        reinterpret_cast<std::uint32_t*>(bitmap->pixels+bitmap->mipOffsets[0]),bitmapDimension,*packedFilterTables);
    dts::updateTexturePixels(texture);
}
std::uint32_t ShapeShadow::textureName() const { return texture?texture->textureName:0; }
void expandPackedFilteredRows(const std::uint8_t* input,std::uint32_t* output,int dimension,const PackedRowFilterTables& table) {
    const int wordsPerRow=dimension>>2; auto* firstRowEnd=output+wordsPerRow;
    do { *output++=0; } while (output<firstRowEnd);
    const auto* upper=input+1; auto* middle=input+(dimension>>3); const auto* lower=input+wordsPerRow+1;
    unsigned topByte=*input,midByte=*middle,lowByte=input[wordsPerRow];
    unsigned topLow=topByte&15,midLow=midByte&15,lowLow=lowByte&15;
    unsigned previousTop=0,previousMid=0,previousLow=0;
    do {
        const unsigned topHigh=topByte>>4,midHigh=midByte>>4,lowHigh=lowByte>>4;
        const unsigned oldTopLow=topLow,oldMidLow=midLow,oldLowLow=lowLow;
        topByte=*upper++; midByte=*++middle; lowByte=*lower++;
        topLow=topByte&15; midLow=midByte&15; lowLow=lowByte&15;
        output[0]=table.previous[previousLow]+(table.previous[previousMid]+table.high[midHigh])*2+
            table.previous[previousTop]+table.low[oldTopLow]+table.high[topHigh]+table.center[oldMidLow]+
            table.low[oldLowLow]+table.high[lowHigh];
        output[1]=table.high[lowLow]+(table.high[midLow]+table.previous[oldMidLow])*2+table.high[topLow]+
            table.previous[oldTopLow]+table.low[topHigh]+table.previous[oldLowLow]+table.low[lowHigh]+table.center[midHigh];
        previousTop=topHigh; previousMid=midHigh; previousLow=lowHigh; output+=2;
    } while (lower<input+(dimension>>3)*dimension-1);
    do { *output++=0; } while (output<firstRowEnd+(dimension-1)*wordsPerRow);
}
extern "C" void engineBitmap_expandPackedFilteredRows_impl(
        const std::uint8_t* input,int* output,int dimension) {
    expandPackedFilteredRows(input,reinterpret_cast<std::uint32_t*>(output),
                             dimension,recoveredPackedFilterTables);
}
namespace {
float coordinate(const Point3F& p,int axis) { return axis==0?p.x:axis==1?p.y:p.z; }
int clipShadowPlane(const Point3F* input,int count,Point3F* output,int axis,float boundary,bool upper) {
    if (!count) return 0;
    Point3F previous=input[count-1]; int written=0;
    const auto inside=[&](const Point3F& p) { return upper?coordinate(p,axis)<boundary:coordinate(p,axis)>boundary; };
    bool previousInside=inside(previous);
    for (int i=0;i<count;++i) {
        const Point3F current=input[i]; const bool currentInside=inside(current);
        if (previousInside!=currentInside) {
            const float distance=coordinate(current,axis)-boundary,delta=coordinate(current,axis)-coordinate(previous,axis);
            Point3F crossing{static_cast<float>(current.x-(current.x-previous.x)*distance/delta),
                static_cast<float>(current.y-(current.y-previous.y)*distance/delta),static_cast<float>(current.z-(current.z-previous.z)*distance/delta)};
            if (!axis) crossing.x=boundary; else if (axis==1) crossing.y=boundary; else crossing.z=boundary;
            output[written++]=crossing;
        }
        if (currentInside) output[written++]=current;
        previous=current; previousInside=currentInside;
    }
    return written;
}
}
void renderGroundShadow(const float* view,float x,float y,float z,float minX,float maxX,float minY,float maxY,float minZ,float maxZ,float alpha,bool disableDepthWrite) {
    if (!shadowObjects || !shadowGrid) throw std::logic_error("Ground shadow scene/grid not bound");
    glColor4f(alpha,alpha,alpha,alpha); glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE); glLoadMatrixf(view);
    if (disableDepthWrite) glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL); glDepthRange(0,0.9999); glDepthFunc(GL_LEQUAL);
    const float left=x+minX,right=x+maxX,bottom=y+minY,top=y+maxY,nearZ=z+minZ,farZ=z+maxZ;
    if (!lookupOrBeginGroundShadow(left,right,bottom,top)) {
        shadowGrid->collectSegmentCandidates(*shadowObjects,{left,bottom,0},{right,top,0});
        auto services=shadowObjects->triangleServices();
        for (int i=0;i<shadowGrid->candidateCount;++i) {
            auto triangle=shadowObjects->triangleStorage[shadowGrid->candidateIds[i]];
            if (shadowObjects->objects[triangle.objectIndex].flags&1) continue;
            Point3F first[32],second[32]; triangle.copyVertices(first,services);
            int count=clipShadowPlane(first,3,second,0,right,true);
            count=clipShadowPlane(second,count,first,0,left,false); count=clipShadowPlane(first,count,second,1,top,true);
            count=clipShadowPlane(second,count,first,1,bottom,false); count=clipShadowPlane(first,count,second,2,farZ,true);
            count=clipShadowPlane(second,count,first,2,nearZ,false);
            if (count) {
                glBegin(GL_POLYGON);
                for (int j=0;j<count;++j) { glTexCoord2f(1-(first[j].x-left)/(right-left),1-(first[j].y-bottom)/(top-bottom)); glVertex3f(first[j].x,first[j].y,first[j].z); }
                glEnd();
            }
        }
        glEndList();
    }
    callCurrentGroundShadow(); glPolygonOffset(0,0); glDisable(GL_POLYGON_OFFSET_FILL); glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS); glDepthRange(0,1); glColor4f(1,1,1,1); glDisable(GL_BLEND);
}
}
