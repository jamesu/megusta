#pragma once
// MG-PLIB-044: modified SSG ASX support, located with the original SSG component.
// Recovered native ownership/inspection types; original private spellings vary.
#include "ssg.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace megusta::asx {
using u32=uint32_t;
using HostMatrix=std::array<std::array<float,4>,4>;
inline HostMatrix identity_matrix() {HostMatrix m{};for(int i=0;i<4;++i)m[i][i]=1;return m;}
inline HostMatrix host_matrix(const sgMat4 a) {HostMatrix m{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)m[c][r]=a[c][r];return m;}
inline void sg_matrix(sgMat4 m,const HostMatrix& a) {for(int c=0;c<4;++c)for(int r=0;r<4;++r)m[c][r]=a[c][r];}
struct aseFace {u32 v[3],tv[3],cv[3],sub_index;};
struct aseMesh {
    u32 num_faces,num_verts,num_tverts,num_cverts;
    aseFace* faces;sgVec3* verts;sgVec2* tverts;sgVec3* cverts;
};
struct aseMaterial {
    std::string name,texture;u32 mat_index,sub_index,sub_flag;
    bool texture_present=false;
    std::array<float,23> floats{};sgVec2 texrep{},texoff{};
};
struct aseVertexBuffer {bool use_flag;int index;sgVec3 v;sgVec2 tv;sgVec3 cv;};
struct Geometry {
    std::vector<std::array<float,3>> vertices,normals;
    std::vector<std::array<float,2>> texcoords;
    std::vector<std::array<float,4>> colours;
    std::vector<uint16_t> indices;
};
template<size_t N> struct ArrayBuilder {
    std::vector<std::array<float,N>>& values;
    void add(const float* p) {std::array<float,N> a;for(size_t i=0;i<N;++i)a[i]=p[i];values.push_back(a);}
};
Geometry build_indexed_geometry(aseMesh*,aseMaterial*);
Geometry expand_triangle_geometry(aseMesh*,aseMaterial*);

// Analyst class name: observed game-derived transform 0047db30/0047dc30,
// original private spelling unknown. Uses the real PLIB transform base.
class GameAnimatedTransform : public ssgTransform {
    std::vector<HostMatrix> frames;
    std::unique_ptr<ssgTransformArray> owned_array; // MG-PLIB-081
    int current=0;
    HostMatrix frame_at(size_t i) const {
        return owned_array ? host_matrix(*owned_array->get(unsigned(i))) : frames[i];
    }
public:
    explicit GameAnimatedTransform(std::vector<HostMatrix> data):frames(std::move(data)) {
        type|=0x1000000; // MG-PLIB-045: original animated-transform runtime flag.
        dirtyBSphere();
    }
    explicit GameAnimatedTransform(ssgTransformArray* array):owned_array(array) {
        if(array)for(int i=0;i<array->getNum();++i)frames.push_back(host_matrix(*array->get(i)));
        type|=0x1000000;dirtyBSphere();
    }
    ~GameAnimatedTransform() override =default;
    void setFrame(int);
    void recalcBSphere() override;
    int frame()const{return current;}
    size_t frameCount()const{return frames.size();}
};
std::vector<HostMatrix> reconstruct_transform_frames(const std::vector<std::array<float,10>>&,const std::array<float,3>&);
struct NodeMetadata {
    std::optional<u32> source_object_index,material_index,sub_index;
    std::string texture;
    u32 primitive_code=8; // Game renderer enum retained as inspection metadata.
};
struct SceneAsset {
    std::unique_ptr<ssgBranch> root_owner=std::make_unique<ssgBranch>();
    std::vector<std::unique_ptr<ssgEntity>> pending;
    ssgBranch* root=root_owner.get();
    std::map<ssgEntity*,NodeMetadata> metadata;
    template<class T,class... Args>T* make(Args&&... args) {
        auto p=std::make_unique<T>(std::forward<Args>(args)...);auto* result=p.get();pending.push_back(std::move(p));return result;
    }
    void attach(ssgBranch* parent,ssgEntity* child) {
        parent->addKid(child);
        for(auto it=pending.begin();it!=pending.end();++it)if(it->get()==child){it->release();pending.erase(it);break;}
    }
    ssgVtxTable* makeLeaf(const Geometry&,bool indexed,u32 primitive=8);
    ssgLeaf* processLeaf(ssgVtxTable*,const char* parent_name); // MG-PLIB-078
    ssgTransform* makeTransform(const HostMatrix& m) {
        auto* p=make<ssgTransform>();sgMat4 a;sg_matrix(a,m);p->setTransform(a);return p;
    }
};
SceneAsset load_asx_scene(const std::string&);
Geometry inspect_geometry(ssgVtxTable*);
}
