#include "spatial_object.h"
#include "geometry_queries.h"
#include "scene_grid.h"
#include "engine/engine_math.h"
#include "engine/opengl_adapter.h"
#include "recovered_binding_primitives.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdlib>
#include <utility>
namespace megusta {
namespace {
long double squared(const Point3F& p) { return static_cast<long double>(p.x)*p.x+static_cast<long double>(p.y)*p.y+static_cast<long double>(p.z)*p.z; }
Point3F subtract(const Point3F& a,const Point3F& b) { return {static_cast<float>(a.x-b.x),static_cast<float>(a.y-b.y),static_cast<float>(a.z-b.z)}; }
}
void SpatialRepel::rasterizeTriangleHeights(Point3F a,Point3F b,Point3F c) {
    if (!(gridMinX_<=a.x || gridMinX_<=b.x || gridMinX_<=c.x) ||
        !(a.x<=gridMaxX_ || b.x<=gridMaxX_ || c.x<=gridMaxX_) ||
        !(gridMinY_<=a.y || gridMinY_<=b.y || gridMinY_<=c.y) || !(a.y<=gridMaxY_ || b.y<=gridMaxY_ || c.y<=gridMaxY_)) return;
    if (b.y<a.y) std::swap(a,b); if (c.y<a.y) std::swap(a,c); if (c.y<b.y) std::swap(b,c);
    if (!(a.y<c.y)) return;
    Point3F left=b,right{static_cast<float>((c.x-a.x)*(b.y-a.y)/(c.y-a.y)+a.x),b.y,
        static_cast<float>((c.z-a.z)*(b.y-a.y)/(c.y-a.y)+a.z)};
    if (right.x<left.x) std::swap(left,right);
    const auto scan=[&](float y,long double x0,long double x1,long double z0,long double z1) {
        float dz=static_cast<float>(x1-x0); if (dz!=0) dz=static_cast<float>((z1-z0)/(x1-x0));
        for (;x0<=x1;x0+=1,z0+=dz) {
            const float gx=static_cast<float>((x0-gridMinX_)*gridScaleX_),gy=(y-gridMinY_)*gridScaleY_;
            if (gx>=0 && gy>=0 && gx<gridColumns_ && gy<gridRows_) {
                auto& cell=heightGrid_[static_cast<int>(gy)*gridColumns_+static_cast<int>(gx)]; if (cell<z0) cell=static_cast<float>(z0);
            }
        }
    };
    float y=a.y-1;
    if (y!=b.y) {
        const float span=b.y-y; long double x0=a.x,x1=a.x,z0=a.z,z1=a.z;
        const float dx0=((left.x-1)-a.x)/span,dx1=((right.x+1)-a.x)/span,dz0=(left.z-a.z)/span,dz1=(right.z-a.z)/span;
        for (;y<=b.y;y+=1) { scan(y,x0,x1,z0,z1); x0+=dx0; x1+=dx1; z0+=dz0; z1+=dz1; }
    }
    if (b.y!=c.y+1) {
        y=c.y+2; const float span=y-b.y; long double x0=c.x,x1=c.x,z0=c.z,z1=c.z;
        const float dx0=(c.x-(left.x-5))/span,dx1=(c.x-(right.x+5))/span,dz0=(c.z-left.z)/span,dz1=(c.z-right.z)/span;
        for (;b.y<=y;y-=1) { scan(y,x0,x1,z0,z1); x0-=dx0; x1-=dx1; z0-=dz0; z1-=dz1; }
    }
}
void SpatialRepel::rebuildHeightGrid(SceneObjectCollection& objects,SceneGridIndex& grid) {
    if (!grid.cellOffsets || !(std::fabs(gridMinX_-(transform_[3][0]-radius_))>=0.1L)) return;
    gridMinX_=transform_[3][0]-radius_; gridMinY_=transform_[3][1]-radius_;
    gridMaxX_=radius_+transform_[3][0]; gridMaxY_=radius_+transform_[3][1];
    const float width=gridMaxX_-gridMinX_; const long double height=static_cast<long double>(gridMaxY_)-gridMinY_;
    gridColumns_=static_cast<int>(width); gridRows_=static_cast<int>(height); if (!gridColumns_ || !gridRows_) return;
    gridScaleX_=static_cast<float>(gridColumns_)/width; gridScaleY_=static_cast<float>(gridRows_)/static_cast<float>(height);
    std::free(heightGrid_); heightGrid_=static_cast<float*>(std::malloc(gridColumns_*gridRows_*sizeof(float)));
    for (int i=0;i<gridColumns_*gridRows_;++i) heightGrid_[i]=-999;
    auto services=objects.triangleServices();
    for (int i=0;i<objects.triangleCount;++i) {
        auto& triangle=objects.triangleStorage[i]; const unsigned flags=objects.objects[triangle.objectIndex].flags;
        if (!(flags&8) && (flags&4) && !(flags&1)) { Point3F vertices[3]; triangle.copyVertices(vertices,services); rasterizeTriangleHeights(vertices[0],vertices[1],vertices[2]); }
    }
    for (int x=0;x<gridColumns_;++x) for (int y=0;y<gridRows_;++y) {
        const int index=y*gridColumns_+x; if (heightGrid_[index]!=-999) continue;
        const float values[4]={static_cast<float>(x?heightGrid_[index-1]:-999),static_cast<float>(x<gridColumns_-1?heightGrid_[index+1]:-999),
            static_cast<float>(y?heightGrid_[index-gridColumns_]:-999),static_cast<float>(y<gridRows_-1?heightGrid_[index+gridColumns_]:-999)};
        float height=-999; int count=0;
        for (float value:values) if (value!=-999) { height=height!=-999?(height+value)*0.5f:value; ++count; }
        if (count>2) heightGrid_[index]=height;
    }
}
void SpatialRepel::applyCameraConstraint(Point3F& eye,const Point3F& focus,const Point3F& aux,
        float& pitch,float& distance,float& heading,const CameraConstraintServices& services) {
    const unsigned kind=flags_&0xff00; Point3F first,second,start;
    if (kind==0x100) { if (eye.z<transform_[3][2]) { eye.z=transform_[3][2]; pitch=eye.z-focus.z; } return; }
    if (kind==0x300) { Point3F output; adjustOrbitAgainstHeightGrid(eye,focus,aux,output,pitch,distance,heading,1,services); return; }
    int intersections;
    const Point3F center{transform_[3][0],transform_[3][1],transform_[3][2]};
    if (!kind) {
        if (radius_*radius_<=squared(subtract(center,eye))) return;
        start={focus.x,focus.y,static_cast<float>(focus.z+200)}; intersections=intersectRaySphere(start,eye,center,radius_,first,second);
    } else if (kind==0x200) {
        start={focus.x,focus.y,static_cast<float>(focus.z+20)};
        intersections=intersectSegmentOrientedBox(start,eye,center,{radius_,radius_,radius_},transform_,first,second);
    } else return;
    if (!intersections) return;
    if (intersections==2 && squared(subtract(second,start))<=squared(subtract(first,start))) first=second;
    const Point3F delta=subtract(first,kind?start:focus); if (std::fabs(delta.x)<1e-6L && std::fabs(delta.y)<1e-6L && std::fabs(delta.z)<1e-6L) return;
    const float square=static_cast<float>(squared(delta)); const long double length=std::sqrt(static_cast<long double>(square));
    if (kind) distance=static_cast<float>(length);
    // Saved _CIacos operand is absent. The adjacent horizontal/total lengths
    // supply the geometric pitch interpretation; this remains comparison pending.
    const float horizontalSquare=delta.x*delta.x+delta.y*delta.y;
    const long double angle=std::acos(std::sqrt(static_cast<long double>(horizontalSquare))/length)*57.29578L;
    if (pitch<angle) pitch=static_cast<float>(angle);
    if (services.globalPitchDegrees<=5) services.globalPitchDegrees=5; else if (services.globalPitchDegrees>80) services.globalPitchDegrees=80;
    if (services.globalFollowDistance<10) services.globalFollowDistance=10;
    else if (!services.isEditing(services.context) && services.globalFollowDistance>130) services.globalFollowDistance=130;
}
namespace {
void wireCube(float radius) {
    const int corners[24][3]={{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1},
        {-1,1,-1},{1,1,-1},{1,1,1},{-1,1,1},{1,-1,-1},{1,1,-1},{1,1,1},{1,-1,1},
        {-1,-1,-1},{-1,1,-1},{-1,1,1},{-1,-1,1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1},
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1}};
    glBegin(GL_QUADS); for (const auto& corner:corners) glVertex3f(corner[0]*radius,corner[1]*radius,corner[2]*radius); glEnd();
}
}
void SpatialRepel::drawDebug(OpenGLAdapter& gl,const float* view,int filter,SpatialObjects& objects,float ballHeight) const {
    if (filter && filter!=2) return;
    gl.matrixMode(0xe); gl.loadIdentity(); gl.loadMatrix(view); gl.multiplyMatrix(&transform_[0][0]);
    gl.disable(0x18); gl.disable(0x2e); gl.disable(0x4a); gl.polygonMode(0x15,0x11);
    if (objects.validateSelection()==this) gl.color4f(0.8f,0.8f,(std::rand()&255)*0.00390625f,1); else gl.color4f(0.9f,0.6f,0.4f,1);
    gl.drawSphere(1,10,5); const unsigned kind=flags_&0xff00;
    if (!kind) gl.drawSphere(radius_+1,8,8); else if (kind==0x100) gl.drawTorus(1,10,radius_,10);
    else if (kind==0x200 || kind==0x300) wireCube(radius_);
    if (kind==0x300) {
        gl.enable(0x4a); gl.matrixMode(0xe); gl.loadIdentity(); gl.loadMatrix(view);
        gl.disable(0x18); gl.disable(0x2e); gl.disable(0x4a); gl.enable(0x21);
        if (heightGrid_) {
            const auto sample=[&](float x,float y) {
                const float gx=(x-gridMinX_)*gridScaleX_,gy=(y-gridMinY_)*gridScaleY_;
                return gx<0 || gy<0 || gx>=gridColumns_ || gy>=gridRows_?-999.0f:heightGrid_[static_cast<int>(gy)*gridColumns_+static_cast<int>(gx)];
            };
            gl.polygonMode(0x15,0x11); glBegin(GL_QUADS);
            for (float x=gridMinX_;x<=gridMaxX_-16;x+=16) for (float y=gridMinY_;y<=gridMaxY_-16;y+=16) {
                float heights[4]={static_cast<float>(sample(x,y)),static_cast<float>(sample(x+16,y)),static_cast<float>(sample(x+16,y+16)),static_cast<float>(sample(x,y+16))};
                gl.color4f(1,1,1,1); bool missing=false; for (float z:heights) if (z==-999) missing=true;
                if (missing) { heights[0]=heights[1]=ballHeight-10; heights[2]=heights[3]=ballHeight+10; gl.color4f(1,0,1,1); }
                glVertex3f(x,y,heights[0]+1); glVertex3f(x+16,y,heights[1]+1); glVertex3f(x+16,y+16,heights[2]+1); glVertex3f(x,y+16,heights[3]+1);
            }
            glEnd();
        }
        gl.polygonMode(0x15,0x12); gl.enable(0x18); gl.enable(0x2e); gl.enable(0x4a);
    }
    gl.enable(0x18); gl.polygonMode(0x15,0x12); gl.enable(0x4a);
}
void SpatialCamera::drawDebug(OpenGLAdapter& gl,const float* view,int filter) const {
    if (filter && filter!=5) return;
    gl.disable(0x18); gl.disable(0x2e); gl.disable(0x4a); gl.matrixMode(0xe); gl.loadIdentity(); gl.loadMatrix(view); gl.multiplyMatrix(&transform_[0][0]);
    const unsigned mode=flags_&0xff00;
    if (mode<=0x300) {
        gl.color4f((std::rand()&255)*0.00390625f,1,1,1);
        if (mode==0x100) { gl.polygonMode(0x15,0x11); gl.drawTorus(1,10,2,10); gl.polygonMode(0x15,0x12); }
        else if (mode==0x200 || mode==0x300) { gl.polygonMode(0x15,0x11); gl.drawCylinder(radius_,radius_,10,10,5); gl.polygonMode(0x15,0x12); if (mode==0x300) gl.drawTorus(1,10,2,10); }
        else gl.drawSphere(2,10,5);
    } else if (mode==0x400) { gl.color4f(1,0,0,1); gl.drawCylinder(radius_,radius_,10,10,5); }
    else if (mode==0x500 || mode==0x600 || mode==0x700) { gl.color4f(mode==0x700?0.5f:0.0f,0.5f,mode==0x500?0.5f:0.0f,1); gl.drawSphere(2,10,5); }
    gl.enable(0x18);
}
void SpatialTarget::drawDebug(OpenGLAdapter& gl,const float* view,int filter) const {
    if (filter && filter!=3) return;
    gl.matrixMode(0xe); gl.loadIdentity(); gl.loadMatrix(view); gl.multiplyMatrix(&transform_[0][0]);
    gl.disable(0x18); gl.disable(0x2e); gl.disable(0x4a); gl.polygonMode(0x15,0x11); gl.color4f(0,0,1,1); gl.drawSphere(radius_+1,8,8);
    gl.enable(0x18); gl.polygonMode(0x15,0x12); gl.enable(0x4a);
}
void SpatialRegion::drawDebug(OpenGLAdapter& gl,const float* view,int filter,const SpatialObjects& objects) const {
    if (filter && filter!=3 && filter!=5) return;
    gl.matrixMode(0xe); gl.loadIdentity(); gl.loadMatrix(view); gl.multiplyMatrix(&transform_[0][0]);
    gl.disable(0x18); gl.polygonMode(0x15,0x11); gl.color4f(1,0,1,1); gl.drawSphere(radius_,12,12);
    gl.color4f(1,1,0,1); gl.drawSphere(1,12,12); gl.polygonMode(0x15,0x12); gl.enable(0x18);
    for (int i=0;i<8;++i) if (auto* other=objects.findByIdPointer(&linkedIds[i])) {
        const float* p=other->position(); const float x=(position()[0]-p[0])*0.5f,y=(position()[1]-p[1])*0.5f,z=(position()[2]-p[2])*0.5f;
        if (std::fabs(x)<1e-6L && std::fabs(y)<1e-6L && std::fabs(z)<1e-6L) continue;
        gl.disable(0x4a); gl.matrixMode(0xe); gl.loadIdentity(); gl.multiplyMatrix(view); glTranslatef(x+p[0],y+p[1],z+p[2]);
        const float horizontal=x*x+y*y;
        const float horizontalLength=binding_math_sqrtf(horizontal);
        long double angle=std::asin(static_cast<long double>(y)/horizontalLength)*
            static_cast<long double>(57.29578f);
        if (x<0) angle=180-angle;
        glRotatef(static_cast<float>(angle),0,0,1); glRotatef(90,1,0,0); gl.color4f(i*0.1f,0.7f,i*0.1f,1);
        gl.drawTorus(0.3f,10,binding_math_sqrtf(z*z+horizontal),10);
    }
}
}
