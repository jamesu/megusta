// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "sg.h"
#include <cstdint>
namespace megusta {
class OpenGLAdapter;
struct Point3F;
class SpatialRepel;
class SpatialObjects;
class Random;
struct SpatialDebugServices;
struct CameraConstraintServices {
    void* context;
    float& globalPitchDegrees;
    float& globalFollowDistance;
    bool (*isEditing)(void*);
    void (*rebuildHeightGrid)(void*, SpatialRepel&);
};
// Shared spatial-object state; original offsets are tracked in recovery exports.
class SpatialObject {
    friend class SpatialObjects;
    friend class Camera;
    friend class SpatialEditor;
public:
    virtual ~SpatialObject() = default;
    virtual const char* typeTag() const = 0;
    virtual int writeRecord(char*,std::uint32_t) const = 0;
    virtual int parseRecord(const char*,std::uint32_t) = 0;
    // Original spatial-object cleanup dispatches this hook over the full list
    // before deleting an object. Most spatial types have no relationships.
    virtual void removeLinkedObject(const SpatialObject*) {}
    void adjustRadiusLimited(float delta);
    void adjustRadius(float delta);
    virtual void translate(float x, float y, float z);
    float* position() { return transform_[3]; }
    const float* position() const { return transform_[3]; }
    float radius() const { return radius_; }
    std::uint32_t flags() const { return flags_; }
    void cycleFlags();
    std::int32_t spatialId() const { return spatialId_; }
protected:
    std::int32_t spatialId_ = 0;
    SpatialObject* next_;
    float radius_ = 1.0f;
    sgMat4 transform_ = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
    std::uint32_t flags_ = 0;
};
class SpatialRegion : public SpatialObject {
public:
    const char* typeTag() const override;
    void removeLinkedObject(const SpatialObject* other) override { removeLinkedId(other); }
    void removeLinkedId(const SpatialObject* other);
    void addLinkedId(const SpatialObject* other);
    int writeRecord(char*,std::uint32_t) const override;
    int parseRecord(const char*,std::uint32_t) override;
    void drawDebug(OpenGLAdapter&,const float*,int,const SpatialObjects&) const;
    std::int32_t linkedIds[8]{};
};
class SpatialTarget : public SpatialObject {
public:
    const char* typeTag() const override;
    int writeRecord(char*,std::uint32_t) const override;
    int parseRecord(const char*,std::uint32_t) override;
    void drawDebug(OpenGLAdapter&,const float*,int) const;
};
class SpatialZone : public SpatialObject {
public:
    const char* typeTag() const override;
    int writeRecord(char*,std::uint32_t) const override;
    int parseRecord(const char*,std::uint32_t) override;
    void drawDebug(OpenGLAdapter&,const float* cameraMatrix,int typeFilter) const;
};
class SpatialObjects {
public:
    SpatialObject *head=nullptr,*selected=nullptr,*iterator=nullptr,*lastCreated=nullptr;
    SpatialObject* findByIdPointer(const int*) const;
    static SpatialObject* createFromTag(const char*);
    void insert(SpatialObject*);
    bool loadMapFile(const char* mapName,const char* extension);
    bool saveMapFile(const char* mapName,const char* extension) const;
    void saveAndDestroy(const char* mapName,const char* extension);
    void drawAllDebug(const SpatialDebugServices&);
    void drawSelectedDebug(const SpatialDebugServices&);
    SpatialObject* validateSelection();
    SpatialObject* next(const SpatialObject* object) const { return object?object->next_:nullptr; }
};
extern SpatialObjects gameSpatialObjects;
void loadCurrentSpatialMap(const char* extension);
struct SpatialDebugServices {
    OpenGLAdapter& gl;
    void* context;
    void (*draw)(void*,SpatialObject&);
    void (*selectedInfo)(void*,SpatialObject&);
};
class SpatialCamera : public SpatialObject {
    friend class Camera;
    friend class SpatialEditor;
public:
    SpatialCamera();
    const char* typeTag() const override;
    int writeRecord(char* text, std::uint32_t capacity) const override;
    int parseRecord(const char* text, std::uint32_t capacity) override;
    void cycleMode(void (*showMessage)(const char*));
    void drawDebug(OpenGLAdapter&,const float*,int) const;
private:
    float lookAt_[3];
};
class SpatialRepel : public SpatialObject {
public:
    SpatialRepel() = default;
    ~SpatialRepel() override;
    SpatialRepel(const SpatialRepel&) = delete;
    SpatialRepel& operator=(const SpatialRepel&) = delete;
    const char* typeTag() const override;
    void addEulerRotation(float x, float y, float z);
    int writeRecord(char* text, std::uint32_t capacity) const override;
    int parseRecord(const char* text, std::uint32_t capacity) override;
    void cycleConstraintKind(void (*showMessage)(const char*));
    bool testPointHeightClearance(const float* point) const;
    void rasterizeTriangleHeights(Point3F,Point3F,Point3F);
    void rebuildHeightGrid(struct SceneObjectCollection&,class SceneGridIndex&);
    void applyCameraConstraint(Point3F& eye,const Point3F& focus,const Point3F& aux,
        float& pitch,float& distance,float& heading,const CameraConstraintServices&);
    void drawDebug(OpenGLAdapter&,const float*,int,SpatialObjects&,float ballHeight) const;
    bool adjustOrbitAgainstHeightGrid(const Point3F& eye, const Point3F& focus,
        const Point3F& target, Point3F& eyeOut, float& pitch, float& distance,
        const float& heading, std::uint32_t mode, const CameraConstraintServices& services);
private:
    float eulerAnglesDegrees_[3] = {};
    // The original constructor leaves these grid bounds/scales/dimensions
    // unwritten. rebuildHeightGrid observes that state before the first build.
    float gridMinX_, gridMaxX_, gridMinY_, gridMaxY_, gridScaleX_, gridScaleY_;
    std::int32_t gridColumns_, gridRows_;
    float* heightGrid_ = nullptr;
    int unresolvedField90_ = 0;
};
}
