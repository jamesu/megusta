// MG-PLIB-076: reconstructed executable ssgConvertASX.cxx writer paths.
#include "ssgConvertASX.h"
#include "ul.h"
#include <cstring>
#include <string>
#include <vector>
#include <cctype>
#include <limits>
#include <cstdlib>

namespace {
FILE* output=nullptr;std::string extension,texture_dir;
bool embed=false;uint32_t material_count=0,object_count=0,embedded_count=0;long scene_end=0;
uint32_t pointer(const void* p){return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));}
void write_exact(const void* p,size_t n) {
  // Original 004b5b10 uses the engine fatal-exit service, independently of
  // any PLIB error callback. The target runtime supplies exit(1).
  if(!output || (n && std::fwrite(p,1,n,output)!=n)) std::exit(1);
}
uint32_t aligned_size(const char* p) {return p ? (uint32_t(std::strlen(p))+4u)&~3u : 0;}
void write_string(const char* p) {
  if(!p)return;size_t n=std::strlen(p)+1;write_exact(p,n);
  const char zero=0;while(n++%4)write_exact(&zero,1);
}
std::string texture_name(const char* p) {
  if(!p)return {};
  const char* slash=std::strrchr(p,'\\');std::string name=slash ? slash+1 : p;
  auto dot=name.rfind('.');if(!extension.empty() && extension[0]=='.' && dot!=std::string::npos) name.replace(dot,std::string::npos,extension);
  return name;
}
bool pix(const _ssgAsxMaterialView& mat,bool check_only) {
  if(!mat.texture || !*mat.texture)return check_only;
  std::string name=mat.texture;auto dot=name.rfind('.');
  if(dot==std::string::npos){ulSetError(UL_WARNING,"ASX Embed Pix: Missing material extension");return false;}
  if(!extension.empty() && extension[0]=='.')name.replace(dot,std::string::npos,extension);
  if(name.find_first_of("/\\")!=std::string::npos){ulSetError(UL_WARNING,"ASX Embed Pix: Material name has path");return false;}
  std::string suffix=name.substr(dot);for(char& c:suffix)c=char(std::tolower((unsigned char)c));
  if(suffix!=".pix"){ulSetError(UL_WARNING,"ASX Embed Pix: Material name is not PIX");return false;}
  std::string path=texture_dir+"/"+name;FILE* in=std::fopen(path.c_str(),"rb");
  if(!in){std::printf("Skipping Embed: %s\n",name.c_str());return false;}
  if(check_only){std::fclose(in);return true;}
  std::fseek(in,0,SEEK_END);long length=std::ftell(in);std::rewind(in);
  if(length<0 || static_cast<unsigned long>(length)>std::numeric_limits<uint32_t>::max())ulSetError(UL_FATAL,"ASX Embed Pix: invalid length");
  name.resize(dot);if(name.size()>14)ulSetError(UL_FATAL,"ASX Embed Pix: Material name too long");
  uint32_t record[7]={0xfeedface,0,uint32_t(length),0,0,0,0};std::memcpy(record+3,name.c_str(),name.size()+1);
  write_exact(record,sizeof(record));
  for(long i=0;i<length;++i){int c=std::fgetc(in);if(c==EOF)ulSetError(UL_FATAL,"Error embedding PIX");unsigned char b=(unsigned char)c;write_exact(&b,1);}
  std::fclose(in);return true;
}
}
void _ssgConfigureAseConversion(FILE* f,const char* ext,const char* dir,bool e){
  output=f;extension=ext?ext:"";texture_dir=dir?dir:"";embed=e;
  material_count=object_count=embedded_count=0;scene_end=0;
}
bool _ssgAseConversionEnabled(){return output!=nullptr;}
uint32_t _ssgAseConvertedObjectCount(){return object_count;}
uint32_t _ssgAseEmbeddedTextureCount(){return embedded_count;}
long _ssgAseConvertedSceneEnd(){return scene_end;}
void _ssgWriteAseObject(const _ssgAsxMaterialView* materials,uint32_t count,const _ssgAsxNodeView& node){
  if(!material_count){material_count=count;for(uint32_t i=0;i<count;++i){
    const auto& m=materials[i];std::string texture=texture_name(m.texture);
    uint32_t w[23]={};w[0]=aligned_size(m.name);w[1]=m.index;w[2]=m.sub_index;w[3]=m.sub_flag;
    std::memcpy(w+4,m.ambient,16);std::memcpy(w+8,m.diffuse,16);std::memcpy(w+12,m.specular,16);
    std::memcpy(w+16,&m.shine,4);std::memcpy(w+17,&m.transparency,4);w[18]=aligned_size(texture.c_str());
    std::memcpy(w+19,m.repeat,8);std::memcpy(w+21,m.offset,8);
    const uint32_t marker=0x20304050;write_exact(&marker,4);write_exact(w,sizeof(w));write_string(m.name);write_string(texture.c_str());
  }}
  ++object_count;const uint32_t marker=0x60708090;write_exact(&marker,4);
  uint32_t w[272]={};w[0]=node.kind;w[1]=aligned_size(node.name);w[2]=aligned_size(node.parent);
  std::memcpy(w+3,node.inherit,12);std::memcpy(w+6,node.position,12);std::memcpy(w+9,node.target,12);
  w[12]=node.material;w[13]=node.sample_count;w[14]=pointer(node.samples);
  for(int i=0;i<256;++i)w[15+i]=pointer(node.meshes[i]);w[271]=node.mesh_count;
  write_exact(w,sizeof(w));write_string(node.name);write_string(node.parent);write_exact(node.samples,size_t(node.sample_count)*40);
  uint32_t seen=0;for(auto* m:node.meshes)if(m){
    uint32_t h[8]={m->faces_count,m->vertices_count,m->texcoords_count,m->colours_count,pointer(m->faces),pointer(m->vertices),pointer(m->texcoords),pointer(m->colours)};
    write_exact(h,sizeof(h));write_exact(m->faces,size_t(m->faces_count)*40);write_exact(m->vertices,size_t(m->vertices_count)*12);
    write_exact(m->texcoords,size_t(m->texcoords_count)*8);write_exact(m->colours,size_t(m->colours_count)*12);++seen;
  }
  if(seen!=node.mesh_count)std::exit(1);
}
void _ssgFinishAseConversion(const _ssgAsxMaterialView* materials,uint32_t count){
  if(output){const uint32_t marker=0x76543210;write_exact(&marker,4);scene_end=std::ftell(output);}
  if(!embed)return;bool all=true;for(uint32_t i=0;i<count;++i)if(!pix(materials[i],true))all=false;
  if(all)for(uint32_t i=0;i<count;++i)if(pix(materials[i],false))++embedded_count;
  std::printf("%u PIX embedded\n",embedded_count);
}
