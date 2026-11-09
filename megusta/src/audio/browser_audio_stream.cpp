#include "browser_audio_stream.h"
#include "wave.h"
#include "engine/resource_file.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstring>
namespace megusta::audio {
namespace {
unsigned u16(const unsigned char* p){return p[0]|(unsigned(p[1])<<8);}
unsigned u32(const unsigned char* p){return u16(p)|(u16(p+2)<<16);}
struct Stream {
    std::unique_ptr<ResourceFile> file;
    unsigned tag=0,channels=0,rate=0,bits=0,align=0,samplesPerBlock=0,dataOffset=0,dataBytes=0,frames=0;
    std::vector<unsigned char> bytes;
    std::vector<short> block;
    std::vector<float> output;
};
std::unordered_map<Handle,std::unique_ptr<Stream>> streams;
unsigned lastCount=0;
bool readExact(ResourceFile& file,void* data,unsigned bytes){return file.read(data,1,bytes)==bytes;}
bool parse(Stream& s){
    auto& f=*s.file;f.seek(0,SEEK_END);const int size=f.tell();f.seek(0,SEEK_SET);
    unsigned char riff[12];if(size<12 || !readExact(f,riff,12) || std::memcmp(riff,"RIFF",4) || std::memcmp(riff+8,"WAVE",4))return false;
    unsigned offset=12;
    while(offset+8u<=static_cast<unsigned>(size)){
        unsigned char header[8];f.seek(offset,SEEK_SET);if(!readExact(f,header,8))return false;
        const unsigned length=u32(header+4);offset+=8;
        if(length>static_cast<unsigned>(size)-offset)return false;
        if(!std::memcmp(header,"fmt ",4)){
            unsigned char fmt[20]={};if(length<16 || !readExact(f,fmt,std::min(length,20u)))return false;
            s.tag=u16(fmt);s.channels=u16(fmt+2);s.rate=u32(fmt+4);s.align=u16(fmt+12);s.bits=u16(fmt+14);
            if(s.tag==17 && length>=20)s.samplesPerBlock=u16(fmt+18);
        }else if(!std::memcmp(header,"data",4)){s.dataOffset=offset;s.dataBytes=length;}
        if(length==static_cast<unsigned>(size)-offset)break;
        const unsigned padded=length+(length&1);if(padded>static_cast<unsigned>(size)-offset)break;offset+=padded;
    }
    if(!s.dataOffset || s.rate<8000 || s.rate>96000 || (s.channels!=1 && s.channels!=2))return false;
    if(s.tag==1){
        if((s.bits!=8 && s.bits!=16) || s.align!=s.channels*(s.bits/8))return false;
        s.frames=s.dataBytes/s.align;
    }else if(s.tag==17){
        if(s.bits!=4 || s.align<4*s.channels || (s.align-4*s.channels)%(4*s.channels))return false;
        const unsigned expected=(s.align-4*s.channels)*2/s.channels+1;
        if(!s.samplesPerBlock || s.samplesPerBlock>expected)return false;
        const unsigned long long frames=static_cast<unsigned long long>(s.dataBytes/s.align)*s.samplesPerBlock;
        if(frames>0xffffffffu)return false;s.frames=static_cast<unsigned>(frames);
    }else return false;
    return s.frames!=0;
}
bool decodeBlock(Stream& s,unsigned blockIndex){
    s.bytes.resize(s.align);s.file->seek(s.dataOffset+blockIndex*s.align,SEEK_SET);
    if(!readExact(*s.file,s.bytes.data(),s.align))return false;
    s.block.assign(s.samplesPerBlock*s.channels,0);
    int predictor[2]={},index[2]={};
    for(unsigned c=0;c<s.channels;++c){
        const auto* p=s.bytes.data()+c*4;predictor[c]=static_cast<short>(u16(p));index[c]=p[2];
        if(index[c]>88)return false;s.block[c]=static_cast<short>(predictor[c]);
    }
    if(s.channels==1){
        unsigned frame=1;
        for(unsigned p=4;p<s.align && frame<s.samplesPerBlock;++p)
            for(int shift=0;shift<=4 && frame<s.samplesPerBlock;shift+=4){
                wave::decodeNibble((s.bytes[p]>>shift)&15,&predictor[0],&index[0]);s.block[frame++]=predictor[0];
            }
    }else{
        unsigned frame=1;
        for(unsigned p=8;p+8<=s.align && frame<s.samplesPerBlock;p+=8,frame+=8)
            for(unsigned c=0;c<2;++c)for(unsigned i=0;i<8 && frame+i<s.samplesPerBlock;++i){
                wave::decodeNibble((s.bytes[p+c*4+i/2]>>((i&1)*4))&15,&predictor[c],&index[c]);
                s.block[(frame+i)*2+c]=predictor[c];
            }
    }
    return true;
}
}
bool registerBrowserStream(Handle h,ResourceFile* file,BrowserStreamInfo& info){
    auto s=std::make_unique<Stream>();s->file.reset(file);
    if(!file || !parse(*s))return false;
    info={s->channels,s->rate,s->frames};streams[h]=std::move(s);return true;
}
void releaseBrowserStream(Handle h){streams.erase(h);}
void releaseBrowserStreams(){streams.clear();}
extern "C" EMSCRIPTEN_KEEPALIVE unsigned megusta_web_stream_count(){return lastCount;}
extern "C" EMSCRIPTEN_KEEPALIVE const float* megusta_web_stream_read(unsigned h,unsigned frame,unsigned requested){
    lastCount=0;auto it=streams.find(h);if(it==streams.end())return nullptr;
    auto& s=*it->second;if(frame>=s.frames || !requested)return nullptr;
    const unsigned count=std::min({requested,s.frames-frame,std::max(1u,s.rate*3/16)});
    s.output.resize(count*s.channels);
    if(s.tag==1){
        s.bytes.resize(count*s.align);s.file->seek(s.dataOffset+frame*s.align,SEEK_SET);
        if(!readExact(*s.file,s.bytes.data(),s.bytes.size()))return nullptr;
        for(unsigned i=0;i<count*s.channels;++i)s.output[i]=s.bits==8?(int(s.bytes[i])-128)/128.0f:static_cast<short>(u16(s.bytes.data()+i*2))/32768.0f;
    }else{
        unsigned written=0;
        while(written<count){
            const unsigned blockIndex=frame/s.samplesPerBlock,within=frame%s.samplesPerBlock;
            if(!decodeBlock(s,blockIndex))return nullptr;
            const unsigned take=std::min(count-written,s.samplesPerBlock-within);
            for(unsigned i=0;i<take*s.channels;++i)s.output[written*s.channels+i]=s.block[within*s.channels+i]/32768.0f;
            written+=take;frame+=take;
        }
    }
    lastCount=count;return s.output.data();
}
}
