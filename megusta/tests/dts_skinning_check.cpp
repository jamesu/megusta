#include "../src/dts/mesh.h"
#include "../src/engine/opengl_adapter.h"
#include <cassert>

namespace megusta {
bool skinTestLighting=true;
bool OpenGLAdapter::isEnabled(int token) { assert(token==0x67); return skinTestLighting; }
}

int main() {
    using namespace megusta::dts;
    TSSkinMesh skin;
    skin.initialVertices={{2.0f,4.0f,6.0f}};
    skin.initialNormals={{0.0f,1.0f,0.0f}};
    skin.initialTransforms.resize(2);
    skin.nodeIndices={0,1};
    skin.vertexIndices={0,0};
    skin.boneIndices={0,1};
    skin.weights={0.25f,0.75f};
    DTSMatrix4 nodes[2]{};
    for (int i=0;i<2;++i) {
        skin.initialTransforms[i].values[0]=skin.initialTransforms[i].values[5]=
            skin.initialTransforms[i].values[10]=skin.initialTransforms[i].values[15]=1.0f;
        nodes[i].values[0]=nodes[i].values[5]=nodes[i].values[10]=nodes[i].values[15]=1.0f;
    }
    nodes[0].values[3]=4.0f;
    nodes[1].values[3]=8.0f;
    megusta::OpenGLAdapter adapter(static_cast<const std::uint32_t*>(nullptr));
    dtsRenderGL=&adapter;
    dtsCurrentNodeTransforms=nodes;
    dtsEnvironmentMode=1;
    skin.verticesPerFrame=1;
    skin.updateSkin();
    assert(skin.boneTransforms.size()==2);
    assert(skin.skinnedVertices.size()==1 && skin.skinnedNormals.size()==1);
    assert(skin.skinnedVertices[0].x==9.0f);
    assert(skin.skinnedVertices[0].y==4.0f && skin.skinnedVertices[0].z==6.0f);
    assert(skin.skinnedNormals[0].x==0.0f && skin.skinnedNormals[0].y==1.0f && skin.skinnedNormals[0].z==0.0f);
    // Base consumers must see the redirected streams after skinning; the
    // owning input vectors are intentionally empty in this fixture.
    const TSMesh& base=skin;
    assert(base.vertexData()==skin.skinnedVertices.data());
    assert(base.normalData()==skin.skinnedNormals.data());
    assert(base.getNormals(0)==skin.skinnedNormals.data());
    assert(base.vertexCount()==1);
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    float bounds[6];
    assert(base.computeBoundsFrame(identity,0,bounds,nullptr,nullptr));
    assert(bounds[0]==9.0f && bounds[3]==9.0f);
    // The saved near-zero branch scales by length, rather than preserving
    // or normalizing the tiny input normal.
    skin.initialNormals[0]={0.0f,0.005f,0.0f};
    skin.updateSkin();
    assert(skin.skinnedNormals[0].y>0.000024f && skin.skinnedNormals[0].y<0.000026f);
    const auto preserved=skin.skinnedNormals[0];
    megusta::skinTestLighting=false;
    dtsEnvironmentMode=0;
    nodes[0].values[3]=8.0f;
    skin.updateSkin();
    assert(base.vertexData()==skin.skinnedVertices.data());
    assert(base.vertexData()[0].x==10.0f);
    assert(base.normalData()[0].y==preserved.y);
}
