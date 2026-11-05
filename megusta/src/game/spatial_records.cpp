#include "spatial_object.h"
#include "engine/opengl_adapter.h"
#include <cstdio>
#include <cstring>
namespace megusta {
void SpatialRegion::addLinkedId(const SpatialObject* other) {
    if (other && other!=this) {
        for (int id:linkedIds) if (id==other->spatialId()) return;
        for (int& id:linkedIds) if (!id) { id=other->spatialId(); return; }
    }
}
int SpatialRegion::parseRecord(const char* text,std::uint32_t) {
    float x,y,z;
    std::sscanf(text+std::strlen(typeTag()),"%d,%f,%f,%f,%f,%d,%d,%d,%d,%d,%d,%d,%d",
        &spatialId_,&x,&y,&z,&radius_,linkedIds,linkedIds+1,linkedIds+2,linkedIds+3,
        linkedIds+4,linkedIds+5,linkedIds+6,linkedIds+7);
    sgMakeIdentMat4(transform_); transform_[3][0]=x; transform_[3][1]=y; transform_[3][2]=z;
    return 1;
}
int SpatialRegion::writeRecord(char* text,std::uint32_t) const {
    std::strcpy(text,typeTag());
    std::sprintf(text+std::strlen(typeTag())," %d, %f, %f, %f, %f, %d, %d, %d, %d, %d, %d, %d, %d",
        spatialId_,static_cast<double>(transform_[3][0]),static_cast<double>(transform_[3][1]),
        static_cast<double>(transform_[3][2]),static_cast<double>(radius_),linkedIds[0],linkedIds[1],
        linkedIds[2],linkedIds[3],linkedIds[4],linkedIds[5],linkedIds[6],linkedIds[7]);
    return static_cast<int>(std::strlen(text));
}
int SpatialTarget::parseRecord(const char* text,std::uint32_t) {
    float x,y,z;
    std::sscanf(text+std::strlen(typeTag()),"%d,%f,%f,%f,%f",&spatialId_,&x,&y,&z,&radius_);
    sgMakeIdentMat4(transform_); transform_[3][0]=x; transform_[3][1]=y; transform_[3][2]=z;
    return 1;
}
int SpatialTarget::writeRecord(char* text,std::uint32_t) const {
    std::strcpy(text,typeTag());
    std::sprintf(text+std::strlen(typeTag())," %d, %f, %f, %f, %f,",spatialId_,
        static_cast<double>(transform_[3][0]),static_cast<double>(transform_[3][1]),
        static_cast<double>(transform_[3][2]),static_cast<double>(radius_));
    return static_cast<int>(std::strlen(text));
}
int SpatialZone::parseRecord(const char* text,std::uint32_t) {
    float x,y,z;
    int flags=static_cast<int>(flags_);
    std::sscanf(text+std::strlen(typeTag()),"%d,%f,%f,%f,%f,%d",&spatialId_,&x,&y,&z,&radius_,&flags);
    flags_=static_cast<std::uint32_t>(flags);
    sgMakeIdentMat4(transform_); transform_[3][0]=x; transform_[3][1]=y; transform_[3][2]=z;
    return 1;
}
int SpatialZone::writeRecord(char* text,std::uint32_t) const {
    std::strcpy(text,typeTag());
    std::sprintf(text+std::strlen(typeTag())," %d, %f, %f, %f, %f, %d",spatialId_,
        static_cast<double>(transform_[3][0]),static_cast<double>(transform_[3][1]),
        static_cast<double>(transform_[3][2]),static_cast<double>(radius_),static_cast<int>(flags_));
    return static_cast<int>(std::strlen(text));
}
void SpatialZone::drawDebug(OpenGLAdapter& gl,const float* cameraMatrix,int typeFilter) const {
    if (!typeFilter || typeFilter==5) {
        gl.matrixMode(0xe); gl.loadIdentity(); gl.loadMatrix(cameraMatrix); gl.multiplyMatrix(&transform_[0][0]);
        gl.disable(0x18); gl.disable(0x2e); gl.disable(0x4a); gl.polygonMode(0x15,0x11);
        gl.color4f(0,0,0,1); gl.drawSphere(radius_+1.0f,8,8);
        gl.enable(0x18); gl.polygonMode(0x15,0x12); gl.enable(0x4a);
    }
}
SpatialObject* SpatialObjects::findByIdPointer(const int* id) const {
    for (auto* object=head;object;object=object->next_) if (object->spatialId_==*id) return object;
    return nullptr;
}
}
