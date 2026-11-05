#include "ssg.h"
#include "ssgASX.h"
#include "megusta_ase_support.h"
#include "ssgConvertASX.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <limits>
extern bool (*_ssgEmbeddedPixLoader)(const char*,ssgTextureInfo*);
struct CountingOptions: ssgLoaderOptions {
    int calls=0,ends=0;bool names=true,texture_context_closed=true;
    ssgLeaf* createLeaf(ssgLeaf* leaf,const char* name) override {
        ++calls;names=names && name && !std::strcmp(name,"ani");
        return ssgLoaderOptions::createLeaf(leaf,name);
    }
    void endLoad() override {++ends;texture_context_closed=texture_context_closed && !_ssgEmbeddedPixLoader;ssgLoaderOptions::endLoad();}
};
int main() {
    ssgBranch root;
    root.setSpare(42);
    auto* child=new ssgBranch;
    child->setName("child");
    root.addKid(nullptr);
    root.addKid(child);
    if(root.getNumKids()!=1 || child->getNumParents()!=1 || child->getRef()!=1) return 1;
    if(root.getByName((char*)"child")!=child) return 2;
    std::unique_ptr<ssgBranch> copy(static_cast<ssgBranch*>(root.clone(0)));
    if(copy->getNumKids()!=1 || child->getNumParents()!=2 || child->getRef()!=2) return 3;
    if(copy->getRef()!=0 || copy->getSpare()!=42 || copy->getType()!=root.getType())return 6;
    root.removeKid(nullptr);
    root.removeAllKids();
    if(child->getNumParents()!=1 || child->getRef()!=1) return 4;
    copy->removeAllKids();
    ssgVertexArray vertices;
    sgVec3 p={1,2,3};vertices.add(p);
    if(vertices.getNum()!=1 || vertices.get(0)[2]!=3) return 5;
    float origin[3]={1,2,3},target[3]={1,5,3},endpoint[3]={};
    if(!_ssgAseCameraEndpoint(origin,target,endpoint) || endpoint[0]!=1 || endpoint[1]!=3 || endpoint[2]!=3)return 34;
    if(_ssgAseCameraEndpoint(origin,origin,endpoint))return 35;
    {
        megusta::asx::aseFace faces[2]{};faces[0].sub_index=2;faces[1].sub_index=1;
        sgVec3 point[1]={{1,2,3}},colour[1]={{0.1f,0.2f,0.3f}};
        sgVec2 uv[1]={{std::numeric_limits<float>::quiet_NaN(),0}};
        megusta::asx::aseMesh mesh{2,1,1,1,faces,point,uv,colour};
        megusta::asx::aseMaterial material{};material.sub_flag=1;material.sub_index=1;
        material.texrep[0]=material.texrep[1]=1;
        auto split=megusta::asx::build_indexed_geometry(&mesh,&material);
        if(split.vertices.size()!=3 || split.indices.size()!=3 || split.indices[2]!=2)return 44;
        uv[0][0]=0;
        auto merged=megusta::asx::build_indexed_geometry(&mesh,&material);
        if(merged.vertices.size()!=1 || merged.indices.size()!=3 || merged.indices[2]!=0)return 45;
    }
    {
        ssgLoaderOptions factory;
        auto* unchanged=new ssgTransform;
        if(factory.createTransform(unchanged,nullptr)!=unchanged)return 39;
        delete unchanged;
        auto* array=new ssgTransformArray;sgMat4 matrix;sgMakeIdentMat4(matrix);array->add(matrix);
        auto* generated=dynamic_cast<megusta::asx::GameAnimatedTransform*>(factory.createTransform(new ssgTransform,array));
        if(!generated)return 40;
        (*array->get(0))[0][0]=2.0f;generated->setFrame(0);generated->getTransform(matrix);
        if(matrix[0][0]!=2.0f)return 41;
        delete generated;
    }
    const char* ase_path="ase-material-cleanup-test.ase";
    FILE* ase=std::fopen(ase_path,"wb");
    if(!ase)return 7;
    std::fputs("*3DSMAX_ASCIIEXPORT 200\n*SCENE {\n*SCENE_FIRSTFRAME 0\n*SCENE_LASTFRAME 1\n*SCENE_TICKSPERFRAME 160\n}\n*MATERIAL_LIST {\n*MATERIAL 0 {\n"
               "*MATERIAL_NAME \"test\"\n*MAP_DIFFUSE {\n*BITMAP \"test.png\"\n}\n}\n"
               "*MATERIAL 1 {\n*MATERIAL_NAME \"plain\"\n*MATERIAL_DIFFUSE 0.2 0.3 0.4\n*MATERIAL_TRANSPARENCY 0.25\n}\n}\n"
               "*GEOMOBJECT {\n*NODE_NAME \"Triangle\"\n*MATERIAL_REF 1\n*MESH {\n*TIMEVALUE 0\n"
               "*MESH_NUMFACES 1\n*MESH_NUMVERTEX 3\n*MESH_VERTEX 0 0 0 0\n*MESH_VERTEX 1 1 0 0\n*MESH_VERTEX 2 0 1 0\n"
               "*MESH_FACE 0: A: 0 B: 1 C: 2 *MESH_MTLID 0\n}\n}\n"
               "*CAMERAOBJECT {\n*NODE_NAME \"Cam\"\n*NODE_TM {\n*NODE_NAME \"Cam\"\n"
               "*TM_POS 1 2 3\n}\n*NODE_TM {\n*NODE_NAME \"Cam.Target\"\n*TM_POS 1 5 3\n}\n"
               "*TM_ANIMATION {\n*NODE_NAME \"Other\"\n*CONTROL_POS_SAMPLE 160 9 9 9\n}\n}\n"
               "*HELPEROBJECT {\n*NODE_NAME \"Animated\"\n*TM_ANIMATION {\n*NODE_NAME \"Animated\"\n"
               "*CONTROL_SCALE_SAMPLE 160 2 3 4\n}\n}\n"
               "*HELPEROBJECT {\n*NODE_NAME \"spotaim\"\n*NODE_TM {\n*NODE_NAME \"spotaim\"\n*TM_POS 1 2 3\n}\n}\n",ase);
    if(std::fclose(ase))return 8;
    const char* converted_path="ase-converted-test.asx";
    FILE* converted=std::fopen(converted_path,"w+b");if(!converted)return 24;
    unsigned converted_header[7]={0x1234abd1,0x78620,0,0,2,0,0};
    std::fwrite(converted_header,4,7,converted);
    _ssgConfigureAseConversion(converted,nullptr,".",false);
    ssgEntity* loaded=ssgLoadASE(ase_path);
    converted_header[3]=(unsigned)_ssgAseConvertedSceneEnd();
    converted_header[5]=_ssgAseConvertedObjectCount();converted_header[6]=_ssgAseEmbeddedTextureCount();
    std::rewind(converted);std::fwrite(converted_header,4,7,converted);std::fclose(converted);
    _ssgConfigureAseConversion(nullptr,nullptr,nullptr,false);
    std::remove(ase_path);
    if(!loaded)return 9;
    auto* tri_node=static_cast<ssgBranch*>(loaded->getByName((char*)"Triangle"));
    if(!tri_node || tri_node->getNumKids()!=1)return 19;
    auto* ase_triangle=static_cast<ssgVtxTable*>(tri_node->getKid(0));
    if(ase_triangle->getNumVertices()!=3 || ase_triangle->getNumColours()!=1 ||
       ase_triangle->getColour(0)[0]!=1.0f || ase_triangle->getColour(0)[3]!=0.75f)return 20;
    auto* camera=static_cast<ssgBranch*>(loaded->getByName((char*)"Cam"));
    if(!camera || camera->getNumKids()!=1)return 10;
    auto* points=static_cast<ssgVtxTable*>(camera->getKid(0));
    if(points->getNumVertices()!=2 || points->getVertex(0)[1]!=2 || points->getVertex(1)[1]!=3)return 11;
    auto* animated=dynamic_cast<megusta::asx::GameAnimatedTransform*>(loaded->getByName((char*)"Animated"));
    if(!animated)return 21;
    if(animated->frameCount()!=2)return 22;
    animated->setFrame(1);sgMat4 animated_matrix;animated->getTransform(animated_matrix);
    if(animated_matrix[0][0]!=2.0f || animated_matrix[1][1]!=3.0f)return 22;
    auto* aim=dynamic_cast<ssgTransform*>(loaded->getByName((char*)"spotaim"));
    if(!aim || aim->getNumKids()!=1 || !dynamic_cast<ssgCutout*>(aim->getKid(0)))return 23;
    if(converted_header[5]!=4 || converted_header[3]<=28)return 25;
    auto converted_scene=megusta::asx::load_asx_scene(converted_path);std::remove(converted_path);
    auto* converted_triangle=static_cast<ssgBranch*>(converted_scene.root->getByName((char*)"Triangle"));
    if(!converted_triangle || converted_triangle->getNumKids()!=1)return 26;
    FILE* pix=std::fopen("embed-test.pix","wb");if(!pix)return 27;
    std::fwrite("abc",1,3,pix);std::fclose(pix);
    FILE* embedded=std::fopen("embedded-test.asx","w+b");if(!embedded)return 28;
    _ssgAsxMaterialView embedded_mat{};embedded_mat.texture="embed-test.pix";
    _ssgConfigureAseConversion(embedded,nullptr,".",true);
    _ssgFinishAseConversion(&embedded_mat,1);
    if(_ssgAseEmbeddedTextureCount()!=1 || _ssgAseConvertedSceneEnd()!=4)return 29;
    std::rewind(embedded);unsigned embedded_words[8]={};
    if(std::fread(embedded_words,4,8,embedded)!=8 || embedded_words[0]!=0x76543210 ||
       embedded_words[1]!=0xfeedface || embedded_words[3]!=3)return 30;
    char payload[3];if(std::fread(payload,1,3,embedded)!=3 || std::memcmp(payload,"abc",3))return 31;
    std::fclose(embedded);_ssgConfigureAseConversion(nullptr,nullptr,nullptr,false);
    embedded=std::fopen("embedded-test.asx","w+b");if(!embedded)return 32;
    _ssgAsxMaterialView mixed[2]{};mixed[0].texture="embed-test.pix";mixed[1].texture="missing.pix";
    _ssgConfigureAseConversion(embedded,nullptr,".",true);_ssgFinishAseConversion(mixed,2);
    if(_ssgAseEmbeddedTextureCount()!=0 || std::ftell(embedded)!=4)return 33;
    std::fclose(embedded);_ssgConfigureAseConversion(nullptr,nullptr,nullptr,false);
    std::remove("embed-test.pix");std::remove("embedded-test.asx");
    delete loaded;
    // Synthetic binary scene with two mesh frames, testing selector topology.
    const char* asx_path="asx-two-frames-test.asx";
    FILE* asx=std::fopen(asx_path,"wb"); if(!asx)return 12;
    auto word=[&](unsigned v){std::fwrite(&v,4,1,asx);};
    unsigned header[]={0x1234abd1,0x78620,0,0,1,1,0};
    std::fwrite(header,4,7,asx); word(0x20304050);
    unsigned mat[23]={}; mat[0]=4;mat[2]=9999;float transparency=0.25f;std::memcpy(&mat[17],&transparency,4);
    std::fwrite(mat,4,23,asx);std::fwrite("mat",1,4,asx);
    word(0x60708090); unsigned object[272]={};object[1]=4;object[271]=2;
    std::fwrite(object,4,272,asx);std::fwrite("ani",1,4,asx);
    for(int frame=0;frame<2;++frame){
        unsigned mesh[8]={1,3,0,0,0,0,0,0};std::fwrite(mesh,4,8,asx);
        unsigned face[10]={0,1,2,0,0,0,0,0,0,0};std::fwrite(face,4,10,asx);
        float points[9]={0,0,0,1,0,0,0,1,float(frame)};std::fwrite(points,4,9,asx);
    }
    word(0x76543210); header[3]=(unsigned)std::ftell(asx);
    std::rewind(asx);std::fwrite(header,4,7,asx);if(std::fclose(asx))return 13;
    ssgLoaderOptions* previous_options=ssgGetCurrentOptions();
    CountingOptions options;ssgSetCurrentOptions(&options);
    auto missing=megusta::asx::load_asx_scene("definitely-missing-test.asx");
    if(!missing.root || missing.root->getNumKids()!=0 || options.ends!=0)return 42;
    auto animation=megusta::asx::load_asx_scene(asx_path);
    if(options.calls!=2 || options.ends!=1 || !options.names)return 36;
    auto* selector=dynamic_cast<ssgSelector*>(animation.root->getByName((char*)"ani"));
    if(!selector || selector->getNumKids()!=2 || !selector->isSelected(0) || selector->isSelected(1))return 14;
    auto* triangle=static_cast<ssgVtxTable*>(selector->getKid(0));
    if(triangle->getNumColours()!=1 || triangle->getColour(0)[0]!=1.0f || triangle->getColour(0)[3]!=0.75f)return 17;
    asx=std::fopen(asx_path,"r+b");if(!asx)return 43;
    std::fseek(asx,125,SEEK_SET);std::fputc(0,asx);std::fseek(asx,127,SEEK_SET);std::fputc('x',asx);std::fclose(asx);
    _ssgUseMaterialTransparency=false;
    auto opaque=megusta::asx::load_asx_scene(asx_path);
    auto* opaque_selector=static_cast<ssgSelector*>(opaque.root->getByName((char*)"ani"));
    auto* opaque_triangle=static_cast<ssgVtxTable*>(opaque_selector->getKid(0));
    if(opaque_triangle->getColour(0)[3]!=1.0f)return 18;
    _ssgUseMaterialTransparency=true;
    asx=std::fopen(asx_path,"r+b");if(!asx)return 37;
    std::fseek(asx,24,SEEK_SET);unsigned one=1;std::fwrite(&one,4,1,asx);
    std::fseek(asx,0,SEEK_END);unsigned embedded_header[7]={0xfeedface,0,0,0,0,0,0};
    std::fwrite(embedded_header,4,7,asx);std::fclose(asx);
    auto with_embedded=megusta::asx::load_asx_scene(asx_path);
    if(!options.texture_context_closed || options.ends!=3)return 38;
    ssgSetCurrentOptions(previous_options);
    asx=std::fopen(asx_path,"r+b");if(!asx)return 15;
    std::fseek(asx,28,SEEK_SET);unsigned bad_tag=0;std::fwrite(&bad_tag,4,1,asx);std::fclose(asx);
    bool rejected=false;try{auto bad=megusta::asx::load_asx_scene(asx_path);}
    catch(const std::runtime_error&){rejected=true;}
    std::remove(asx_path);if(!rejected)return 16;
    std::printf("Distributed PLIB SSG graph: passed; pointer=%u\n",unsigned(sizeof(void*)));
    return 0;
}
