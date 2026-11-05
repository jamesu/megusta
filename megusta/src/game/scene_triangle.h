#pragma once
#include <cstdint>
#include "ssg.h"
namespace megusta {
struct Point3F { float x, y, z; };
// Typed bridge to the scene object array. Implementations supply the same
// lazy transform and flags access used by Object_getFrameTransform_impl.
struct SceneTriangleServices {
    void* context;
    const float* (*frameTransform)(void*, std::uint16_t objectIndex);
    std::uint32_t (*objectFlags)(void*, std::uint16_t objectIndex);
};
struct SceneTriangleRecord {
    ssgLeaf* leaf;
    std::uint16_t leafTriangleIndex, objectIndex;
    float planeNormal[3], planeDistance;
    Point3F vertices[3];
    void initialize(ssgLeaf* source, std::uint16_t triangle, std::uint16_t object, const SceneTriangleServices& scene);
    void refreshVertices(const SceneTriangleServices& scene);
    void refreshPlane(const SceneTriangleServices& scene);
    void copyVertices(Point3F* output, const SceneTriangleServices& scene);
    float copyPlane(float* normal, const SceneTriangleServices& scene);
};
}
