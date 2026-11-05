// Faithful reconstruction of PIX 00462a40/00462ab0 and unpacking00499940.
// Codec bodies00499780/004998b0 have no established upstream origin.
#include "pix_texture.h"
#include "runtime_adapters.h"
#include "ul.h"
#include "zlib.h"
#include "resource_runtime.h"
#include "reader_stream.h"
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>
extern "C" void megusta_glpng_buildMipmaps(GLint,GLint,GLint,GLenum,const unsigned char*,int);

namespace megusta::pix {
namespace {
[[noreturn]] void invalid(){engine_runtime::engineRuntime_fatalExit_impl();}
struct Input {
    const unsigned char* bytes;size_t size,position=0;
    unsigned byte(){if(position>=size)invalid();return bytes[position++];}
};
void copy_back(unsigned char* output,size_t& position,size_t capacity,size_t distance,size_t count){
    if(distance>position || count>capacity-position)invalid();
    for(size_t i=0;i<count;++i){output[position]=output[position-distance];++position;}
}
struct Header {
    uint32_t magic,unknown_04,unknown_08,level_count,descriptor_count,mipmap_mode;
    uint32_t format,component_count,alpha,palette_bytes,packed_capacity,unknown_2c,unpacked_capacity;
};
struct Level {
    int16_t width,height,compression,unknown_06;
    uint32_t packed_bytes,unpacked_bytes;
};
static_assert(sizeof(Header)==52 && sizeof(Level)==16,"PIX disk records");
static int maximum_texture_size;
void unpack(const unsigned char* input,size_t size,unsigned char* output,size_t capacity,int compression){
    size_t decoded=0;
    if(compression==2)decoded=decode_flagged(input,size,output,capacity);
    else if(compression==4)decoded=decode_runs(input,size,output,capacity);
    else if(compression>=5 && compression<=13){
        uLongf length=capacity;
        if(uncompress(output,&length,input,size)!=Z_OK)invalid();
        decoded=length;
    }else{
        ulSetError(UL_WARNING,"Unsupported PIX unpacking used!");
        std::memset(output,0,capacity);return;
    }
    if(decoded!=capacity)invalid();
}
}

size_t decode_flagged(const unsigned char* input,size_t size,unsigned char* output,size_t capacity){
    Input source{input,size};unsigned width=source.byte();uint32_t count=0;
    if(width>1){count=source.byte()<<24;count+=source.byte()<<16;}
    if(width)count+=source.byte()<<8;
    count+=source.byte();
    size_t flags=source.position,flag_bytes=(size_t(count)+7)/8;
    if(flag_bytes>size-source.position)invalid();
    source.position+=flag_bytes;size_t written=0;
    for(uint32_t token=0;token<count && written<capacity;++token){
        if(input[flags+token/8] & (128u>>(token&7))){
            unsigned code=source.byte()<<8;code|=source.byte();
            copy_back(output,written,capacity,(code>>6)+1,std::min<size_t>((code&63)+3,capacity-written));
        }else output[written++]=source.byte();
    }
    return written;
}

size_t decode_runs(const unsigned char* input,size_t size,unsigned char* output,size_t capacity){
    Input source{input,size};size_t written=0;
    do{
        unsigned token=source.byte();
        if(token<32){
            size_t count=token+1;if(count>capacity-written)invalid();
            for(size_t i=0;i<count;++i)output[written++]=source.byte();
        }else{
            size_t count=token>>5;if(count==7)count+=source.byte();
            size_t distance=(token&31)*256+source.byte()+1;
            copy_back(output,written,capacity,distance,count+2);
        }
    }while(written<capacity && source.position<size);
    return written;
}

bool load(FILE* file,TextureInfo* info,bool all_levels){
    if(!file) return load(static_cast<MegustaReader*>(nullptr),info,all_levels);
    auto reader=megustaReaderFromFILE(file);
    return load(&reader,info,all_levels);
}
bool load(MegustaReader* file,TextureInfo* info,bool all_levels){
    if(info)*info={};
    if(!file){ulSetError(UL_WARNING,"Missing PIX file!");return false;}
    Header header{};
    if(file->read(file->context,&header,sizeof(header),1)!=1 || header.magic!=0xdadde4)invalid();
    // Original uses a fixed descriptor stack buffer. Dynamic storage preserves
    // valid records while avoiding its malformed-count stack overrun.
    if(header.descriptor_count<header.level_count)invalid();
    std::vector<Level> levels(header.descriptor_count);
    file->read(file->context,levels.data(),sizeof(Level),levels.size());
    std::vector<unsigned char> palette(header.palette_bytes);
    if(header.palette_bytes){if(header.palette_bytes!=1024)invalid();file->read(file->context,palette.data(),header.palette_bytes,1);}
    std::vector<unsigned char> packed(header.packed_capacity),unpacked(header.unpacked_capacity);
    GLint alignment;glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    unsigned uploaded_level=0;
    for(unsigned index=0;index<header.level_count;++index){
        const Level& level=levels[index];
        if(!maximum_texture_size)glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum_texture_size);
        if((maximum_texture_size<level.width || maximum_texture_size<level.height) && index<header.level_count-1){
            file->seek(file->context,level.packed_bytes,SEEK_CUR);continue;
        }
        if(level.packed_bytes>packed.size())invalid();
        file->read(file->context,packed.data(),level.packed_bytes,1);
        const unsigned char* pixels=packed.data();
        if(level.compression){
            if(level.unpacked_bytes>unpacked.size())invalid();
            unpack(pixels,level.packed_bytes,unpacked.data(),level.unpacked_bytes,level.compression);pixels=unpacked.data();
        }
        int components;GLenum format;
        if(header.format==0){components=4;format=GL_RGBA;}
        else if(header.format==1){components=3;format=GL_RGB;}
        else invalid();
        if(header.mipmap_mode && header.level_count<2){
            megusta_glpng_buildMipmaps(components,level.width,level.height,format,pixels,header.mipmap_mode==1);break;
        }
        glTexImage2D(GL_TEXTURE_2D,uploaded_level++,components,level.width,level.height,0,format,GL_UNSIGNED_BYTE,pixels);
        if(!all_levels)break;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
    if(info && !levels.empty())*info={levels[0].width,levels[0].height,int32_t(header.component_count*8),int32_t(header.alpha),int32_t(header.format),int32_t(header.mipmap_mode)};
    return true;
}
bool load_file(const char* filename,TextureInfo* info,bool all_levels){
    if(info)*info={};
    ResourceFile* file=sharedIo->open(filename,"rb");
    if(!file){ulSetError(UL_WARNING,"Missing PIX file!");return false;}
    MegustaReader reader=resourceReader(*file);
    const bool result=load(&reader,info,all_levels);
    sharedIo->close(file);
    return result;
}
}
