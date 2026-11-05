// RECONSTRUCTED host CLI: traverses nodes built by the ASX construction subset.
#include "ssgASX.h"
#include "engine/resource_runtime.h"
#include "dts/resource_bridge.h"
using namespace megusta::asx;
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <cstring>
int checkNetworkBindings();
int checkPlatformRuntime();
int checkStartupArguments();
int checkPriorityPass();

static void scalar(float value) {
    // JSON has no NaN/Infinity. Preserve undefined normals explicitly as null,
    // rather than inventing a finite normal for degenerate source triangles.
    if (std::isfinite(value)) std::cout << value; else std::cout << "null";
}
template<class T> static void scalar(T value) { std::cout << value; }

template<class T> static void array(const T& values) {
    std::cout << '['; bool first=true;
    for (const auto& value:values) { if (!first) std::cout << ','; first=false; scalar(value); }
    std::cout << ']';
}
template<class T> static void vectors(const T& values) {
    std::cout << '['; bool first=true;
    for (const auto& value:values) { if (!first) std::cout << ','; first=false; array(value); }
    std::cout << ']';
}

static void text(const std::string& s) {
    std::cout << '"';
    for (unsigned char c:s) {
        if (c=='"' || c=='\\') std::cout << '\\' << c;
        else if (c<32) std::cout << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec;
        else std::cout << c;
    }
    std::cout << '"';
}
static void select_frames(ssgEntity* entity,int frame) {
    if (auto* transform=dynamic_cast<GameAnimatedTransform*>(entity))transform->setFrame(frame);
    if (auto* branch=dynamic_cast<ssgBranch*>(entity))
        for (int i=0;i<branch->getNumKids();++i)select_frames(branch->getKid(i),frame);
}
static void node(SceneAsset& scene,ssgEntity* entity,std::set<ssgEntity*>& active,const HostMatrix& parent) {
    if (!active.insert(entity).second) throw std::runtime_error("Scene cycle");
    const auto& info=scene.metadata[entity];
    std::cout << "{\"type\":"; text(dynamic_cast<GameAnimatedTransform*>(entity)?"gameTransform@0047db30":entity->getTypeName()); std::cout << ",\"name\":";
    if (entity->getName()) text(entity->getName()); else std::cout << "null";
    std::cout << ",\"source_object_index\":";
    if (info.source_object_index)std::cout << *info.source_object_index;else std::cout << "null";
    std::cout << ",\"parents\":" << entity->getNumParents() << ",\"refs\":" << entity->getRef();
    std::cout << ",\"traversal_mask\":" << static_cast<uint32_t>(entity->getTraversalMask());
    if (auto* cutout=dynamic_cast<ssgCutout*>(entity))
        std::cout << ",\"point_rotate\":" << cutout->isPointRotate();
    HostMatrix world=parent;
    if (auto* transform=dynamic_cast<ssgBaseTransform*>(entity)) {
        sgMat4 p,l,w;sg_matrix(p,parent);transform->getTransform(l);sgMultMat4(w,p,l);world=host_matrix(w);
        std::cout << ",\"matrix_columns\":";vectors(host_matrix(l));
        if (auto* animated=dynamic_cast<GameAnimatedTransform*>(entity))
            std::cout << ",\"frame_count\":" << animated->frameCount() << ",\"frame\":" << animated->frame()
                      << ",\"reported_type\":\"ssgTransform\",\"binary_vtable\":\"004ebe44\"";
    }
    std::cout << ",\"world_matrix_columns\":";vectors(world);
    if (auto* leaf=dynamic_cast<ssgVtxTable*>(entity)) {
        const bool indexed=dynamic_cast<ssgVtxArray*>(leaf)!=nullptr;
        const auto geometry=inspect_geometry(leaf);
        std::cout << ",\"primitive_code\":" << info.primitive_code
                  << ",\"faces\":" << (info.primitive_code==8 ? (indexed ? geometry.indices.size() : geometry.vertices.size())/3 : 0)
                  << ",\"vertices\":" << geometry.vertices.size() << ",\"texture\":";
        if (info.material_index)text(info.texture);else std::cout << "null";
        std::cout << ",\"material_index\":";
        if (info.material_index)std::cout << *info.material_index;else std::cout << "null";
        std::cout << ",\"sub_index\":";
        if (info.sub_index)std::cout << *info.sub_index;else std::cout << "null";
        std::cout << ",\"geometry\":{\"indexed\":" << (indexed ? "true" : "false") << ",\"vertices\":"; vectors(geometry.vertices);
        std::cout << ",\"normals\":"; vectors(geometry.normals);
        std::cout << ",\"indices\":"; array(geometry.indices);
        std::cout << ",\"texcoords\":"; vectors(geometry.texcoords);
        std::cout << ",\"colours\":"; vectors(geometry.colours); std::cout << '}';
    }
    std::cout << ",\"children\":[";
    if (auto* branch=dynamic_cast<ssgBranch*>(entity)) {
        for (int i=0;i<branch->getNumKids();++i) {
            if (i) std::cout << ',';
            node(scene,branch->getKid(i),active,world);
        }
    }
    std::cout << "]}"; active.erase(entity);
}
int main(int argc,char** argv) {
    if (argc==2 && !std::strcmp(argv[1],"--check-platform")) return checkPlatformRuntime();
    if (argc==2 && !std::strcmp(argv[1],"--check-startup")) return checkStartupArguments();
    if (argc==2 && !std::strcmp(argv[1],"--check-priority-pass")) return checkPriorityPass();
    if (argc==2 && !std::strcmp(argv[1],"--check-network-bindings")) return checkNetworkBindings();
    try {
        megusta::ResourceRuntime resources;
        resources.initializeFromArgs(argc, argv);
        megusta::dts::bindIflResourceService(resources.services().open,resources.services().close);
        if (argc!=2 && argc!=3) throw std::runtime_error("usage: load_scene <shipped ASX> [frame]");
        std::cout << std::setprecision(std::numeric_limits<float>::max_digits10);
        {
            auto scene=load_asx_scene(argv[1]); std::set<ssgEntity*> active;
            if (argc==3)select_frames(scene.root,std::stoi(argv[2]));
            node(scene,scene.root,active,identity_matrix()); std::cout << '\n';
        }

        megusta::dts::bindIflResourceService(nullptr,nullptr);
    } catch (const std::exception& e) {
        megusta::dts::bindIflResourceService(nullptr,nullptr);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
