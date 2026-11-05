#include "ssg.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
static unsigned failures=0;
static void check(bool okay,const char* message){if(!okay){++failures;std::fprintf(stderr,"FAIL %s\n",message);}}
static std::vector<char> read(const char* path){std::ifstream file(path,std::ios::binary);return {(std::istreambuf_iterator<char>(file)),{}};}
static void word(std::vector<char>& bytes,uint32_t value){for(int i=0;i<4;++i)bytes.push_back(char(value>>(8*i)));}
static ssgVtxTable* mesh(bool indexed){
    auto* vertices=new ssgVertexArray;sgVec3 points[3]={{0,0,0},{1,0,0},{0,1,0}};
    for(auto& point:points)vertices->add(point);
    auto* normals=new ssgNormalArray;auto* uv=new ssgTexCoordArray;auto* colors=new ssgColourArray;
    ssgVtxTable* result;
    if(indexed){auto* indices=new ssgIndexArray;for(short i=0;i<3;++i)indices->add(i);result=new ssgVtxArray(GL_TRIANGLES,vertices,normals,uv,colors,indices);}
    else result=new ssgVtxTable(GL_TRIANGLE_STRIP,vertices,normals,uv,colors);
    result->setName(indexed?"indexed":"mesh");result->setRenderOrderMode(7);result->recalcBSphere();return result;
}
int main(){
    {ssgRoot root;root.setName("root");check(ssgSaveSSG("v0-empty.ssg",&root),"save root");
     std::vector<char> expected;for(uint32_t value:{0x53534700u,0x413u,0u,0xffffffffu})word(expected,value);
     for(char c:std::array<char,5>{'r','o','o','t',0})expected.push_back(c);
     check(read("v0-empty.ssg")==expected,"exact empty-root wire bytes");
     std::unique_ptr<ssgEntity> loaded(ssgLoadSSG("v0-empty.ssg"));check(loaded && !std::strcmp(loaded->getName(),"root"),"load root/name");}
    {ssgRoot root;root.setName("tree");auto* transform=new ssgTransform;sgMat4 matrix;sgMakeTransMat4(matrix,2,3,4);transform->setTransform(matrix);root.addKid(transform);
     transform->addKid(mesh(false));auto* selector=new ssgSelector(7);selector->selectStep(0);root.addKid(selector);selector->addKid(mesh(true));
     auto* cutout=new ssgCutout(1);root.addKid(cutout);cutout->addKid(new ssgBranch);
     auto* texture_transform=new ssgTexTrans;root.addKid(texture_transform);
     check(ssgSaveSSG("v0-tree.ssg",&root),"save nested tree");
     std::unique_ptr<ssgEntity> loaded(ssgLoadSSG("v0-tree.ssg"));
     check(loaded && loaded->isAKindOf(ssgTypeRoot()),"load nested root type");
     if(loaded){auto* branch=static_cast<ssgBranch*>(loaded.get());check(branch->getNumKids()==4,"child count/order");
       auto* first=static_cast<ssgBranch*>(branch->getKid(0));auto* geometry=static_cast<ssgVtxTable*>(first->getKid(0));
       check(geometry->getNumVertices()==3 && geometry->getVertex(1)[0]==1 && geometry->getRenderOrderMode()==7,"geometry arrays and extra leaf word");
       auto* choices=static_cast<ssgBranch*>(branch->getKid(1));auto* indexed=static_cast<ssgVtxArray*>(choices->getKid(0));
       check(indexed->getNumIndices()==3 && *indexed->getIndex(2)==2,"index array");
       check(ssgSaveSSG("v0-tree-again.ssg",loaded.get()),"resave tree");
       check(read("v0-tree.ssg")==read("v0-tree-again.ssg"),"nested tree byte-preserving round trip");}}
    {ssgRoot root;auto* shared=new ssgBranch;root.addKid(shared);root.addKid(shared);
     check(ssgSaveSSG("v0-shared.ssg",&root),"shared child writer");
     auto bytes=read("v0-shared.ssg");std::vector<char> suffix;for(uint32_t value:{0x12345678u,0u,2u,0x23456789u})word(suffix,value);
     check(bytes.size()>=suffix.size() && std::equal(suffix.begin(),suffix.end(),bytes.end()-suffix.size()),"shared child backward reference encoding");}
    {auto bytes=read("v0-empty.ssg");bytes[0]=1;std::ofstream file("v1-rejected.ssg",std::ios::binary);file.write(bytes.data(),bytes.size());file.close();
     check(ssgLoadSSG("v1-rejected.ssg")==nullptr,"reject unsupported version1");}
    std::printf("PLIB SSG version0: %u failures; original reader quirks retained\n",failures);return failures?1:0;
}
