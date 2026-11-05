#include "ssgASX.h"
namespace megusta::asx {
ssgVtxTable* SceneAsset::makeLeaf(const Geometry& g,bool indexed,u32 primitive) {
    auto vertices=std::make_unique<ssgVertexArray>();
    // MG-PLIB-078: absent attributes are null, not empty PLIB arrays.
    std::unique_ptr<ssgNormalArray> normals;
    std::unique_ptr<ssgTexCoordArray> texcoords;
    std::unique_ptr<ssgColourArray> colours;
    if(!g.normals.empty())normals=std::make_unique<ssgNormalArray>();
    if(!g.texcoords.empty())texcoords=std::make_unique<ssgTexCoordArray>();
    if(!g.colours.empty())colours=std::make_unique<ssgColourArray>();
    for(auto a:g.vertices)vertices->add(a.data());
    for(auto a:g.normals)normals->add(a.data());
    for(auto a:g.texcoords)texcoords->add(a.data());
    for(auto a:g.colours)colours->add(a.data());
    // MG-PLIB-087: recovered graphics tokens map 4 to GL_POINTS and 8 to
    // GL_TRIANGLES. Retain the original enum separately for inspection.
    GLenum mode=primitive==8?GL_TRIANGLES:GL_POINTS;
    ssgVtxTable* result;
    if(indexed) {
        auto indices=std::make_unique<ssgIndexArray>();
        for(uint16_t i:g.indices){short value;std::memcpy(&value,&i,2);indices->add(value);}
        result=make<ssgVtxArray>(mode,vertices.get(),normals.get(),texcoords.get(),colours.get(),indices.get());
        indices.release();
    } else result=make<ssgVtxTable>(mode,vertices.get(),normals.get(),texcoords.get(),colours.get());
    vertices.release();normals.release();texcoords.release();colours.release();
    metadata[result].primitive_code=primitive;return result;
}
ssgLeaf* SceneAsset::processLeaf(ssgVtxTable* leaf,const char* parent_name) {
    // Original 00463ba0/00463f90 delegate the leaf to the active loader.
    NodeMetadata info=metadata[leaf];
    for(auto it=pending.begin();it!=pending.end();++it)if(it->get()==leaf){it->release();pending.erase(it);break;}
    ssgLeaf* result=ssgGetCurrentOptions()->createLeaf(leaf,parent_name);
    if(result){pending.emplace_back(result);if(result!=leaf){metadata.erase(leaf);metadata[result]=std::move(info);}}
    else metadata.erase(leaf);
    return result;
}
Geometry inspect_geometry(ssgVtxTable* leaf) {
    Geometry g;
    for(int i=0;i<leaf->getNumVertices();++i){auto* p=leaf->getVertex(i);g.vertices.push_back({p[0],p[1],p[2]});}
    for(int i=0;i<leaf->getNumNormals();++i){auto* p=leaf->getNormal(i);g.normals.push_back({p[0],p[1],p[2]});}
    for(int i=0;i<leaf->getNumTexCoords();++i){auto* p=leaf->getTexCoord(i);g.texcoords.push_back({p[0],p[1]});}
    for(int i=0;i<leaf->getNumColours();++i){auto* p=leaf->getColour(i);g.colours.push_back({p[0],p[1],p[2],p[3]});}
    if(auto* indexed=dynamic_cast<ssgVtxArray*>(leaf))
        for(int i=0;i<indexed->getNumIndices();++i)g.indices.push_back(static_cast<uint16_t>(*indexed->getIndex(i)));
    return g;
}
}
