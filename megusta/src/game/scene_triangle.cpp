#include "scene_triangle.h"
#include <cmath>
#include <cstring>
namespace megusta {
void SceneTriangleRecord::initialize(ssgLeaf* source, std::uint16_t triangle, std::uint16_t object, const SceneTriangleServices& scene) {
    leaf = source; leafTriangleIndex = triangle; objectIndex = object;
    refreshPlane(scene);
}
void SceneTriangleRecord::refreshVertices(const SceneTriangleServices& scene) {
    short indices[3];
    leaf->getTriangle(leafTriangleIndex, &indices[0], &indices[1], &indices[2]);
    const float* matrix = scene.frameTransform(scene.context, objectIndex);
    for (int i = 0; i < 3; ++i) {
        const float* p = leaf->getVertex(indices[i]);
        vertices[i].x = p[0]*matrix[0] + p[1]*matrix[4] + p[2]*matrix[8] + matrix[12];
        vertices[i].y = p[0]*matrix[1] + p[1]*matrix[5] + p[2]*matrix[9] + matrix[13];
        vertices[i].z = p[0]*matrix[2] + p[1]*matrix[6] + p[2]*matrix[10] + matrix[14];
    }
}
void SceneTriangleRecord::refreshPlane(const SceneTriangleServices& scene) {
    refreshVertices(scene);
    const float x1=vertices[1].x-vertices[0].x, y1=vertices[1].y-vertices[0].y, z1=vertices[1].z-vertices[0].z;
    const float x2=vertices[2].x-vertices[0].x, y2=vertices[2].y-vertices[0].y, z2=vertices[2].z-vertices[0].z;
    planeNormal[0]=z2*y1-y2*z1;
    planeNormal[1]=z1*x2-z2*x1;
    planeNormal[2]=y2*x1-y1*x2;
    const double length=std::sqrt(static_cast<double>(planeNormal[2])*planeNormal[2] +
        static_cast<double>(planeNormal[1])*planeNormal[1] + static_cast<double>(planeNormal[0])*planeNormal[0]);
    if (length != 0.0) {
        const double inverse=1.0/length;
        for (float& component : planeNormal) component=static_cast<float>(inverse*component);
    }
    planeDistance=-(planeNormal[0]*vertices[0].x + planeNormal[1]*vertices[0].y + planeNormal[2]*vertices[0].z);
}
void SceneTriangleRecord::copyVertices(Point3F* output, const SceneTriangleServices& scene) {
    if (!(scene.objectFlags(scene.context, objectIndex) & 4)) refreshVertices(scene);
    std::memcpy(output, vertices, sizeof(vertices));
}
float SceneTriangleRecord::copyPlane(float* normal, const SceneTriangleServices& scene) {
    if (!(scene.objectFlags(scene.context, objectIndex) & 4)) refreshPlane(scene);
    normal[0]=planeNormal[0]; normal[1]=planeNormal[1]; normal[2]=planeNormal[2];
    return planeDistance;
}
}
