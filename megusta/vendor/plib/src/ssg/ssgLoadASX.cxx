// RECONSTRUCTED-EVIDENCE-DERIVED: ASX reader and game node construction subset.
// Anchors: 0048af50 reader, 00463330 builder, 00464c20 material lookup,
// 00463b20 submaterial count, 00463f90 geometry. Unsupported paths fail explicitly.
#include "ssgASX.h"
#include "megusta_ase_support.h"
#include "ssgLocal.h"
#include "pix_texture.h"
#include "resource_runtime.h"
#include "reader_stream.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>

extern bool _ssgTextureHasAlpha;
namespace megusta::asx {

namespace {
// MG-PLIB-059: appended ASX texture records are live only during scene build.
// Original code keeps one process-global file and searches it on every lookup.
struct EmbeddedTextures {
    megusta::ResourceFile *file=nullptr;
    u32 count=0,offset=0;
    EmbeddedTextures *previous=nullptr;
    bool (*previous_loader)(const char*,ssgTextureInfo*)=nullptr;
    static EmbeddedTextures *current;
    EmbeddedTextures(const std::string& path,u32 n,u32 begin):count(n),offset(begin) {
        if(!count)return;
        if(megusta::sharedIo)file=megusta::sharedIo->open(path.c_str(),"rb");
        else if(auto* native=std::fopen(path.c_str(),"rb"))file=new megusta::ResourceFile(native);
        if(!file)throw std::runtime_error("Cannot reopen ASX embedded textures");
        previous=current;previous_loader=_ssgEmbeddedPixLoader;
        current=this;_ssgEmbeddedPixLoader=lookup;
    }
    void close(){
        if(file){_ssgEmbeddedPixLoader=previous_loader;current=previous;delete file;file=nullptr;}
    }
    ~EmbeddedTextures(){close();}
    static bool lookup(const char *filename,ssgTextureInfo *info){
        if(!current || !current->file)return false;
        std::string name(filename);
        if(name.size()>63)throw std::runtime_error("ASX texture name exceeds 63 bytes");
        auto dot=name.find_last_of('.');if(dot!=std::string::npos)name.resize(dot);
        u32 position=current->offset;
        for(u32 i=0;i<current->count;++i){
            if(current->file->seek(position,SEEK_SET)!=0)throw std::runtime_error("Bad ASX embedded texture offset");
            unsigned char header[28];
            if(current->file->read(header,sizeof(header),1)!=1)throw std::runtime_error("Truncated ASX embedded texture header");
            auto word=[&](int p){return u32(header[p])|(u32(header[p+1])<<8)|(u32(header[p+2])<<16)|(u32(header[p+3])<<24);};
            if(word(0)!=0xfeedface)throw std::runtime_error("Bad ASX embedded texture marker");
            const char *record=reinterpret_cast<const char*>(header+12);
            size_t length=0;while(length<16 && record[length])++length;
            if(length==16)throw std::runtime_error("Unterminated ASX embedded texture name");
            if(ulStrEqual(record,name.c_str())){
                megusta::pix::TextureInfo result{};
                auto reader=megusta::resourceReader(*current->file);
                if(!megusta::pix::load(&reader,&result,true))return false;
                ::_ssgTextureHasAlpha=result.alpha>0;
                if(info){info->width=result.width;info->height=result.height;info->depth=result.bits_per_pixel;info->alpha=result.alpha;}
                return true;
            }
            if(position>UINT32_MAX-28 || word(8)>UINT32_MAX-position-28)throw std::runtime_error("ASX embedded texture offset overflow");
            position+=28+word(8);
        }
        return false;
    }
};
EmbeddedTextures *EmbeddedTextures::current=nullptr;
struct Reader {
    std::vector<unsigned char> bytes;
    size_t pos=0, limit=0;
    bool opened=false;
    explicit Reader(const std::string& path) {
        std::unique_ptr<megusta::ResourceFile> file;
        if(megusta::sharedIo)file.reset(megusta::sharedIo->open(path.c_str(),"rb"));
        else if(auto* native=std::fopen(path.c_str(),"rb"))file.reset(new megusta::ResourceFile(native));
        if (!file) return; // MG-PLIB-082: original returns its empty root.
        opened=true;
        unsigned char block[4096];unsigned count;
        while((count=file->read(block,1,sizeof(block)))!=0)bytes.insert(bytes.end(),block,block+count);
        limit=bytes.size();
    }
    void check(size_t n) const {
        if (pos>limit || n>limit-pos) throw std::runtime_error("Truncated ASX at "+std::to_string(pos));
    }
    // MG-PLIB-072: original 0048b2c0 advances the shared buffer cursor;
    // retain bounds checks and represent an empty span as nullptr.
    const unsigned char* take(size_t n) {
        if(!n) return nullptr;
        check(n); const auto* result=bytes.data()+pos;pos+=n;return result;
    }
    u32 word() {
        const auto* p=take(4);
        return u32(p[0])|(u32(p[1])<<8)|(u32(p[2])<<16)|(u32(p[3])<<24);
    }
    float real() {
        u32 bits=word(); float value; std::memcpy(&value,&bits,4);
        return value;
    }
    void marker(u32 value) {
        // Original 0048b2f0 fatally exits on mismatch; native code throws.
        if (word()!=value) throw std::runtime_error("Bad ASX marker");
    }
    std::optional<std::string> text(u32 n) {
        if (!n) return std::nullopt;
        check(n);
        auto start=pos; auto end=start;
        while (end<start+n && bytes[end]) ++end;
        if (end==start+n) throw std::runtime_error("Unterminated ASX string");
        pos+=n;
        return std::string(reinterpret_cast<const char*>(bytes.data()+start),end-start);
    }
};
struct Frame {
    aseMesh mesh{};
    std::unique_ptr<aseFace[]> faces;
    std::unique_ptr<sgVec3[]> verts,colours;
    std::unique_ptr<sgVec2[]> texcoords;
};
Frame read_frame(Reader& r) {
    Frame f;
    f.mesh.num_faces=r.word(); f.mesh.num_verts=r.word();
    f.mesh.num_tverts=r.word(); f.mesh.num_cverts=r.word();
    for (int i=0;i<4;++i) (void)r.word(); // saved process pointers are not offsets
    r.check(size_t(f.mesh.num_faces)*40+size_t(f.mesh.num_verts)*12+
            size_t(f.mesh.num_tverts)*8+size_t(f.mesh.num_cverts)*12);
    f.faces=std::make_unique<aseFace[]>(f.mesh.num_faces);
    for (u32 i=0;i<f.mesh.num_faces;++i) {
        auto& face=f.faces[i];
        for (auto& v:face.v) v=r.word();
        for (auto& v:face.tv) v=r.word();
        for (auto& v:face.cv) v=r.word();
        face.sub_index=r.word();
        for (int j=0;j<3;++j) {
            if (face.v[j]>=f.mesh.num_verts ||
                (f.mesh.num_tverts && face.tv[j]>=f.mesh.num_tverts) ||
                (f.mesh.num_cverts && face.cv[j]>=f.mesh.num_cverts))
                throw std::runtime_error("ASX face index outside array");
        }
    }
    f.verts=std::make_unique<sgVec3[]>(f.mesh.num_verts);
    for (u32 i=0;i<f.mesh.num_verts;++i) for (float& v:f.verts[i]) v=r.real();
    if (f.mesh.num_tverts) {
        f.texcoords=std::make_unique<sgVec2[]>(f.mesh.num_tverts);
        for (u32 i=0;i<f.mesh.num_tverts;++i) for (float& v:f.texcoords[i]) v=r.real();
    }
    if (f.mesh.num_cverts) {
        f.colours=std::make_unique<sgVec3[]>(f.mesh.num_cverts);
        for (u32 i=0;i<f.mesh.num_cverts;++i) for (float& v:f.colours[i]) v=r.real();
    }
    f.mesh.faces=f.faces.get(); f.mesh.verts=f.verts.get();
    f.mesh.tverts=f.texcoords.get(); f.mesh.cverts=f.colours.get();
    return f;
}
struct Object {
    std::array<u32,272> words;
    std::optional<std::string> name,parent;
    std::vector<Frame> frames;
    std::vector<std::array<float,10>> transforms;
};
void add_helper(SceneAsset& scene,ssgBranch* branch,const Object& object) {
    // RECONSTRUCTED-EVIDENCE-DERIVED helper/camera fallback 00463ba0.
    std::vector<std::array<float,3>> points;
    if (object.words[0]==2) {
        std::array<float,3> position,target,endpoint;
        for (int i=0;i<3;++i) {
            std::memcpy(&position[i],&object.words[6+i],4);
            std::memcpy(&target[i],&object.words[9+i],4);
        }
        points.push_back(position);
        if (_ssgAseCameraEndpoint(position.data(),target.data(),endpoint.data())) points.push_back(endpoint);
    } else if (!object.frames.empty()) {
        const auto& frame=object.frames[0];
        for (u32 i=0;i<frame.mesh.num_verts;++i) {
            const auto& v=frame.mesh.verts[i];points.push_back({v[0],v[1],v[2]});
        }
    }
    if (points.empty()) return;
    Geometry geometry;geometry.vertices=std::move(points);
    auto* leaf=scene.makeLeaf(geometry,false,4);leaf->clrTraversalMaskBits(1);
    auto* result=scene.processLeaf(leaf,object.name ? object.name->c_str() : nullptr);
    if(result)scene.attach(branch,result);
}
aseMaterial* find_material(std::vector<aseMaterial>& materials,u32 index,u32 sub) {
    for (auto& m:materials) if (m.mat_index==index && m.sub_index==sub) return &m;
    for (auto& m:materials) if (m.mat_index==index) return &m;
    ulSetError(UL_WARNING,"unknown material #%d",index);
    return nullptr;
}
u32 count_submaterials(std::vector<aseMaterial>& materials,u32 index) {
    u32 count=0;
    for (auto& m:materials) if (m.mat_index==index) {
        if (!m.sub_flag && ulStrNEqual(m.name.c_str(),"ifl_",4))return 1;
        if (m.sub_flag) ++count;
    }
    return count;
}
ssgSimpleState* material_state(std::vector<aseMaterial>& materials,aseMaterial& material) {
    if(ulStrNEqual(material.name.c_str(),"ifl_",4)){
        u32 frames=0;for(auto& m:materials)if(m.mat_index==material.mat_index && m.sub_flag)++frames;
        if(frames>1){
            auto* selector=new ssgStateSelector(frames);
            for(u32 frame=0;frame<frames;++frame){
                auto* m=find_material(materials,material.mat_index,frame);
                if(!m)throw std::runtime_error("Missing IFL frame material");
                selector->setStep(frame,_ssgBuildAseMaterialState(m->texture_present?m->texture.c_str():nullptr,m->mat_index,m->floats[17]));
            }
            selector->selectStep(0);return selector;
        }
        ulSetError(UL_WARNING,"ifl material only has <2 frames: %s",material.name.c_str());
    }
    return _ssgBuildAseMaterialState(material.texture_present?material.texture.c_str():nullptr,material.mat_index,material.floats[17]);
}
void add_mesh(SceneAsset& scene,ssgBranch* branch,Frame& frame,
              std::vector<aseMaterial>& materials,u32 index,u32 sub,const char* name) {
    auto* mat=find_material(materials,index,sub);
    if (!mat) return; // EXE 00463f90 returns null when lookup fails
    u32 selected=0;
    for (u32 i=0;i<frame.mesh.num_faces;++i)
        if (!mat->sub_flag || frame.mesh.faces[i].sub_index==mat->sub_index) ++selected;
    if (!selected) return;
    Geometry geometry;
    const bool indexed=frame.mesh.cverts!=nullptr;
    auto* state=indexed ? nullptr : material_state(materials,*mat); // MG-PLIB-084
    if(indexed)geometry=build_indexed_geometry(&frame.mesh,mat);
    else {
        geometry=expand_triangle_geometry(&frame.mesh,mat);
        geometry.colours.push_back({1.0f,1.0f,1.0f,_ssgUseMaterialTransparency ? float(1.0f-mat->floats[17]) : 1.0f}); // MG-PLIB-073
    }
    if(indexed)state=material_state(materials,*mat);
    auto* leaf=scene.makeLeaf(geometry,indexed);
    leaf->setState(state);
    auto& info=scene.metadata[leaf];info.texture=mat->texture;
    info.material_index=mat->mat_index;info.sub_index=mat->sub_index;
    auto* result=scene.processLeaf(leaf,name);
    if(result)scene.attach(branch,result);
}
// MG-PLIB-071: 00463ea0 visits branches recursively before flattening leaves.
void flatten_aim(ssgEntity* entity,const std::array<float,3>& pivot) {
    if(auto* branch=dynamic_cast<ssgBranch*>(entity)) {
        for(int kid=0;kid<branch->getNumKids();++kid)flatten_aim(branch->getKid(kid),pivot);
    } else if(auto* leaf=dynamic_cast<ssgVtxTable*>(entity)) {
        for(int vi=0;vi<leaf->getNumVertices();++vi) {
            auto* vertex=leaf->getVertex(vi);
            vertex[0]-=pivot[0];vertex[1]=0.0f;vertex[2]-=pivot[2];
        }
        leaf->dirtyBSphere();
        // MG-PLIB-086: 00463ea0 deletes and recompiles after changing vertices.
#ifdef _SSG_USE_DLIST
        leaf->deleteDList();
        leaf->makeDList();
#endif
    }
}
}

SceneAsset load_asx_scene(const std::string& path) {
    // MG-PLIB-082: 0048af50 joins the global model path before opening.
    char resolved[512];ssgGetCurrentOptions()->ssgLoaderOptions::makeModelPath(resolved,path.c_str());
    SceneAsset scene;
    Reader r(resolved);
    if(!r.opened)return scene;
    if (r.word()!=0x1234abd1 || r.word()!=0x78620)
        throw std::runtime_error("Unsupported ASX header");
    const auto total=r.word(),end=r.word(),material_count=r.word(),object_count=r.word(),embedded=r.word();
    (void)total; // Original reader does not use the third header word as a file-length guard.
    if (end<28 || end>r.bytes.size())
        throw std::runtime_error("Unsupported ASX boundaries");
    r.limit=end;
    EmbeddedTextures embedded_textures(resolved,embedded,end);
    std::vector<aseMaterial> materials;
    for (u32 i=0;i<material_count;++i) {
        r.marker(0x20304050); std::array<u32,23> words; for (auto& w:words) w=r.word();
        aseMaterial m; m.name=r.text(words[0]).value_or("");
        auto texture=r.text(words[18]);m.texture_present=texture.has_value();m.texture=texture.value_or("");
        m.mat_index=words[1]; m.sub_index=words[2]; m.sub_flag=words[3];
        for (int j=0;j<23;++j) std::memcpy(&m.floats[j],&words[j],4);
        for (int j=0;j<2;++j) { m.texrep[j]=m.floats[19+j];m.texoff[j]=m.floats[21+j]; }
        materials.push_back(std::move(m));
    }
    for (u32 i=0;i<object_count;++i) {
        r.marker(0x60708090); Object object;
        for (auto& w:object.words) w=r.word();
        object.name=r.text(object.words[1]); object.parent=r.text(object.words[2]);
        r.check(size_t(object.words[13])*40);
        object.transforms.resize(object.words[13]);
        for (auto& sample:object.transforms)for (auto& value:sample)value=r.real();
        if (object.words[271]>256) throw std::runtime_error("Too many mesh frames");
        for (u32 frame=0;frame<object.words[271];++frame) object.frames.push_back(read_frame(r));
        ssgBranch* branch;
        if(object.frames.size()>1) {
            // MG-PLIB-071: the reader installs mesh records into consecutive
            // slots; the builder makes a selector with one selected step.
            auto* selector=scene.make<ssgSelector>(int(object.frames.size()));
            const auto count=count_submaterials(materials,object.words[12]);
            for(auto& frame:object.frames) {
                if(count<2) add_mesh(scene,selector,frame,materials,object.words[12],9999,object.name ? object.name->c_str() : nullptr);
                else {
                    auto* subbranch=scene.make<ssgBranch>();
                    for(u32 sub=0;sub<count;++sub)add_mesh(scene,subbranch,frame,materials,object.words[12],sub,object.name ? object.name->c_str() : nullptr);
                    scene.attach(selector,subbranch);
                }
            }
            selector->selectStep(0);
            branch=selector;
        } else {
            branch=scene.make<ssgBranch>();
        if (!object.frames.empty() && object.frames[0].mesh.num_faces) {
            const auto count=count_submaterials(materials,object.words[12]);
            if (count<2) add_mesh(scene,branch,object.frames[0],materials,object.words[12],9999,object.name ? object.name->c_str() : nullptr);
            else for (u32 sub=0;sub<count;++sub) add_mesh(scene,branch,object.frames[0],materials,object.words[12],sub,object.name ? object.name->c_str() : nullptr);
        } else add_helper(scene,branch,object);
        }
        // Resolve only against already attached nodes, exactly as 00463330.
        ssgBranch* parent=scene.root;
        if (object.parent) {
            auto* found=scene.root->getByName(object.parent->data());
            if (found) {
                parent=dynamic_cast<ssgBranch*>(found);
                if (!parent) throw std::runtime_error("ASX parent is not a branch");
            }
        }
        ssgBranch* outer=branch;
        std::array<float,3> pivot;
        for (int j=0;j<3;++j)std::memcpy(&pivot[j],&object.words[6+j],4);
        if (object.name && object.name->size()>2 && object.name->compare(object.name->size()-3,3,"aim")==0) {
            // RECONSTRUCTED-EVIDENCE-DERIVED 00463ea0: flatten only geometry
            // already built for this object. Later object children are unaffected.
            flatten_aim(branch,pivot);
            auto* cutout=scene.make<ssgCutout>(1);
            scene.attach(cutout,branch);
            sgMat4 translation;sgMakeTransMat4(translation,pivot.data());
            outer=scene.makeTransform(host_matrix(translation));
            scene.attach(outer,cutout);
        }
        if (!object.transforms.empty()) {
            auto* inner=outer;
            // MG-PLIB-081: invoke the loader's transform factory, which owns
            // both the initial transform and the transform-array argument.
            auto frames=reconstruct_transform_frames(object.transforms,pivot);
            auto* array=new ssgTransformArray(int(frames.size()));
            for(auto& frame:frames){sgMat4 matrix;sg_matrix(matrix,frame);array->add(matrix);}
            auto* result=ssgGetCurrentOptions()->createTransform(new ssgTransform,array);
            if(!result)throw std::runtime_error("ASX transform factory returned null");
            scene.pending.emplace_back(result);outer=result;
            scene.attach(outer,inner);
        }
        scene.attach(parent,outer);
        scene.metadata[outer].source_object_index=i;
        outer->setName(object.name ? object.name->c_str() : nullptr);
    }
    // MG-PLIB-080: the executable closes its embedded stream before the
    // terminator check and releases loader storage before the callback.
    embedded_textures.close();
    r.marker(0x76543210);
    if (!scene.pending.empty()) throw std::runtime_error("Unattached native nodes remain");
    std::vector<aseMaterial>().swap(materials);
    std::vector<unsigned char>().swap(r.bytes);
    ssgGetCurrentOptions()->endLoad(); // MG-PLIB-079: original 0048af50 callback.
    return scene;
}

} // namespace megusta::asx
