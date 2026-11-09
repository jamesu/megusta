#include "audio/browser_audio_stream.h"
#include "engine/resource_file.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
// File-service double keeps this decoder fixture independent of game globals.
namespace megusta {
ResourceFile::~ResourceFile(){if(file_)std::fclose(file_);}
std::uint32_t ResourceFile::read(void* p,std::uint32_t s,std::uint32_t n){return std::fread(p,s,n,file_);}
int ResourceFile::seek(std::int32_t o,int w){return std::fseek(file_,o,w);}
std::int32_t ResourceFile::tell()const{return std::ftell(file_);}
}
extern "C" const float* megusta_web_stream_read(unsigned,unsigned,unsigned);
extern "C" unsigned megusta_web_stream_count();
using namespace megusta;
using namespace megusta::audio;
static void le(std::vector<unsigned char>& v,unsigned x,int n){while(n--){v.push_back(x&255);x>>=8;}}
static void tag(std::vector<unsigned char>& v,const char* p){for(int i=0;i<4;++i)v.push_back(p[i]);}
static ResourceFile* fixture(bool ima,bool invalidIndex=false){
    std::vector<unsigned char> v;tag(v,"RIFF");le(v,0,4);tag(v,"WAVE");tag(v,"fmt ");le(v,ima?20:16,4);
    le(v,ima?17:1,2);le(v,1,2);le(v,22050,4);le(v,22050,4);le(v,ima?8:1,2);le(v,ima?4:8,2);
    if(ima){le(v,2,2);le(v,9,2);}tag(v,"data");le(v,ima?8:4,4);
    if(ima){le(v,0,2);le(v,invalidIndex?99:0,1);le(v,0,1);le(v,0x11111111,4);}
    else{v.push_back(0);v.push_back(128);v.push_back(255);v.push_back(64);}
    unsigned size=v.size()-8;for(int i=0;i<4;++i)v[4+i]=(size>>(i*8))&255;
    auto* f=std::fopen("build/browser-stream-fixture.wav","w+b");assert(f);assert(std::fwrite(v.data(),1,v.size(),f)==v.size());std::rewind(f);return new ResourceFile(f);
}
int main(){
    BrowserStreamInfo info{};assert(registerBrowserStream(1,fixture(false),info));assert(info.frames==4);
    auto* p=megusta_web_stream_read(1,1,2);assert(p && megusta_web_stream_count()==2);assert(p[0]==0 && p[1]==127/128.0f);
    p=megusta_web_stream_read(1,0,1);assert(p && p[0]==-1);
    p=megusta_web_stream_read(1,3,20);assert(p && megusta_web_stream_count()==1 && p[0]==-0.5f);
    assert(!megusta_web_stream_read(1,4,1));releaseBrowserStream(1);assert(!megusta_web_stream_read(1,0,1));
    assert(registerBrowserStream(2,fixture(true),info));assert(info.frames==9);
    p=megusta_web_stream_read(2,1,4);assert(p && megusta_web_stream_count()==4);
    for(int i=0;i<4;++i)assert(std::fabs(p[i]-(i+1)/32768.0f)<1e-7f);
    releaseBrowserStream(2);assert(registerBrowserStream(3,fixture(true,true),info));
    assert(!megusta_web_stream_read(3,0,9) && megusta_web_stream_count()==0);
    releaseBrowserStreams();std::puts("PASS: incremental PCM/IMA decode, seeking, EOF, released handles and malformed IMA block");
}
