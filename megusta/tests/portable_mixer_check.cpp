#include "audio/portable_mixer.h"
#include "engine/resource_file.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
namespace megusta {
ResourceFile::~ResourceFile(){if(file_)std::fclose(file_);}
std::uint32_t ResourceFile::read(void* p,std::uint32_t s,std::uint32_t n){return std::fread(p,s,n,file_);}
int ResourceFile::seek(std::int32_t o,int w){return std::fseek(file_,o,w);}
std::int32_t ResourceFile::tell()const{return std::ftell(file_);}
}
using namespace megusta::audio;
static bool near(float a,float b){return std::fabs(a-b)<0.002f;}
int main(){
    PortableMixer m;Handle b,s;m.genBuffers(1,&b);m.genSources(1,&s);
    unsigned char pcm[8]={192,192,192,192,192,192,192,192};
    m.bufferData(b,Mono8,pcm,8,22050);m.sourcei(s,BufferBinding,b);m.sourcei(s,SourceRelative,1);
    float out[32];m.play(s);m.render(out,1,22050);assert(near(out[0],0.5f)&&near(out[1],0.5f));
    m.sourcef(s,Pan,1);m.play(s);m.render(out,1,22050);assert(out[0]==0&&near(out[1],0.5f));
    m.sourcef(s,Pan,0.5f);m.play(s);m.render(out,1,22050);assert(near(out[0],0.25f)&&near(out[1],0.5f));
    m.sourcef(s,Pan,0);m.sourcef(s,Gain,0.5f);m.play(s);m.render(out,1,22050);assert(near(out[0],0.25f));
    m.sourcef(s,Gain,1);m.sourcei(s,SourceRelative,0);m.source3f(s,Position,0,0,2);
    const float centre=std::sqrt(0.5f);
    m.play(s);m.render(out,1,22050);assert(near(out[0],0.25f*centre)&&near(out[1],0.25f*centre));
    m.source3f(s,Position,0,0,4);m.play(s);m.render(out,1,22050);assert(near(out[0],0.125f*centre));
    m.distanceScale(0.3048f);m.play(s);m.render(out,1,22050);assert(near(out[0],0.125f*centre));
    m.sourcef(s,MaxDistance,2);m.play(s);m.render(out,1,22050);assert(near(out[0],0.25f*centre));
    m.sourcef(s,MaxDistance,1e9f);m.distanceScale(1);
    // With the recovered Y/Z mapping, game +X is right when facing +Y.
    m.source3f(s,Position,2,0,0);m.play(s);m.render(out,1,22050);assert(out[0]==0&&near(out[1],0.25f));
    const float forwardY[6]={0,0,1,0,1,0};m.listenerfv(Orientation,forwardY);m.listenerf(CommitDeferred,0);
    m.render(out,1,22050);assert(out[0]==0&&near(out[1],0.25f));
    m.source3f(s,Position,-2,0,0);m.play(s);m.render(out,1,22050);assert(near(out[0],0.25f)&&out[1]==0);
    m.source3f(s,Position,0,0,2);m.source3f(s,Direction,0,0,1);m.sourcei(s,ConeOuterAngle,90);
    m.sourcei(s,ConeInnerAngle,0);m.play(s);m.render(out,1,22050);assert(out[0]==0&&out[1]==0);
    m.sourcei(s,ConeOuterAngle,360);m.play(s);m.render(out,1,22050);
    assert(out[0]==0&&out[1]==0); // narrower inner cone still applies
    m.sourcei(s,SourceRelative,1);m.sourcei(s,Looping,1);
    m.play(s);m.render(out,16,22050);int playing=0;m.getSourcei(s,IsPlaying,&playing);assert(playing);
    m.sourcei(s,Looping,0);m.play(s);m.render(out,16,22050);m.getSourcei(s,IsPlaying,&playing);assert(!playing);
    // DirectSound copies ordinary samples into the source; deleting their API
    // buffer does not invalidate a playing sample.
    m.play(s);m.deleteBuffers(1,&b);m.render(out,1,22050);assert(near(out[0],0.5f));
    assert(m.getError()==NoError);m.clear();
    m.genBuffers(1,&b);m.genSources(1,&s);
    std::vector<short> ramp(512);for(int i=0;i<512;++i)ramp[i]=i*32;
    m.bufferData(b,Mono16,ramp.data(),ramp.size()*2,22050);m.sourcei(s,BufferBinding,b);
    m.source3f(s,Position,0,0,1);m.source3f(s,Velocity,0,0,-180);
    std::vector<float> block(256);
    m.play(s);m.render(block.data(),64,22050);assert(near(block[64],0.0625f*centre)); // 2x Doppler
    m.source3f(s,Velocity,0,0,-10000);m.play(s);m.render(block.data(),64,22050);
    assert(near(block[64],0.0625f*centre)); // approaching velocity is capped
    m.source3f(s,Velocity,0,0,10000);m.play(s);m.render(block.data(),64,22050);
    assert(near(block[96],0.03125f*centre)); // receding limit = 2/3 playback rate
    m.source3f(s,Velocity,0,0,-90);m.listener3f(Velocity,0,0,90);m.listenerf(CommitDeferred,0);
    m.play(s);m.render(block.data(),64,22050);assert(near(block[64],0.0625f*centre));
    m.source3f(s,Velocity,0,0,90);m.play(s);m.render(block.data(),64,22050);
    assert(near(block[64],0.03125f*centre)); // common motion cancels
    m.listener3f(Velocity,0,0,0);m.listenerf(CommitDeferred,0);m.source3f(s,Velocity,0,0,0);
    m.bufferData(b,Mono8,pcm,8,22050);m.sourcei(s,Looping,1);
    // Deferred position changes must leave the currently playing mix intact.
    m.play(s);m.source3f(s,Position,1,0,0);m.render(out,1,22050);
    assert(near(out[0],0.5f*centre)&&near(out[1],0.5f*centre));
    m.listenerf(CommitDeferred,0);m.render(out,1,22050);assert(out[0]==0&&near(out[1],0.5f));
    m.listener3f(Position,1,0,-1);m.render(out,1,22050);assert(out[0]==0);
    m.listenerf(CommitDeferred,0);m.render(out,1,22050);assert(near(out[0],0.5f*centre));
    m.listener3f(Position,0,0,0);m.listenerf(CommitDeferred,0);
    // Angular power distribution: diagonal = 25% / 75%, not linear balance.
    m.source3f(s,Position,centre,0,centre);m.play(s);m.render(out,1,22050);
    assert(near(out[0],0.25f)&&near(out[1],0.5f*std::sqrt(0.75f)));
    assert(near(out[0]*out[0]+out[1]*out[1],0.25f));
    m.sourcei(s,ConeInnerAngle,90);m.sourcei(s,ConeOuterAngle,90);
    m.source3f(s,Direction,1,0,0);m.play(s);m.render(out,1,22050);assert(near(out[0],0.25f));
    // A listener rotation swaps the spatial channels only after commit.
    const float orientation[6]={0,0,1,0,-1,0};m.listenerfv(Orientation,orientation);
    m.render(out,1,22050);assert(near(out[0],0.25f));m.listenerf(CommitDeferred,0);
    m.render(out,1,22050);assert(near(out[1],0.25f));
    const float forwardX[6]={0,0,1,1,0,0};m.listenerfv(Orientation,forwardX);m.listenerf(CommitDeferred,0);
    m.source3f(s,Position,0,-1,0);m.play(s);m.render(out,1,22050);assert(out[0]==0&&near(out[1],0.5f));
    m.source3f(s,Position,0,1,0);m.play(s);m.render(out,1,22050);assert(near(out[0],0.5f)&&out[1]==0);
    m.clear();m.genBuffers(1,&b);m.genSources(1,&s);m.sourcei(s,SourceRelative,1);
    m.sourcei(s,Looping,1);m.sourcei(s,BufferBinding,b);
    std::vector<short> tone(8192);
    for(unsigned i=0;i<tone.size();++i)tone[i]=(i&1)?8192:-8192;
    m.bufferData(b,Mono16,tone.data(),tone.size()*2,44100);m.play(s);m.render(block.data(),128,22050);
    for(float value:block)assert(std::abs(value)<0.005f); // reject above output Nyquist
    for(unsigned i=0;i<tone.size();++i)tone[i]=static_cast<short>(8192*std::sin(2*3.141592653589793*1000*i/44100));
    m.bufferData(b,Mono16,tone.data(),tone.size()*2,44100);m.play(s);m.render(block.data(),128,22050);
    double energy=0;for(unsigned i=16;i<128;++i)energy+=block[i*2]*block[i*2];
    assert(std::abs(std::sqrt(energy/112)-0.25/std::sqrt(2.0))<0.01); // preserve audible-band level
    m.clear();
    assert(mixerGainUnits(0)==-10000&&mixerGainUnits(1)==0&&mixerPanUnits(1)==10000);
    std::puts("PASS: 2D balance, spatial power, distance, cones, deferred updates, relative Doppler, filtered resampling and lifetimes");
}
