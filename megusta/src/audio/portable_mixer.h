// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "openal.h"
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
namespace megusta::audio {
struct MixerLevels { float left,right;double doppler; };
int mixerGainUnits(double);
int mixerPanUnits(double);
class PortableMixer {
public:
    void clear();
    void fail(int code){if(!error)error=code;}
    int getError(){const int result=error;error=0;return result;}
    unsigned genSources(unsigned,Handle*);
    unsigned genBuffers(unsigned,Handle*);
    void deleteSources(unsigned,const Handle*);
    void deleteBuffers(unsigned,const Handle*);
    bool isBuffer(Handle h)const{return buffers.count(h)!=0;}
    void bufferData(Handle,int,const void*,unsigned,unsigned);
    void bufferStream(Handle,ResourceFile*);
    void sourcei(Handle,int,int);
    void sourcef(Handle,int,float);
    void source3f(Handle,int,float,float,float);
    void getSourcei(Handle,int,int*);
    void play(Handle);
    void stop(Handle);
    void listenerf(int,float);
    void listener3f(int,float,float,float);
    void listenerfv(int,const float*);
    void distanceScale(float);
    void render(float* stereo,unsigned frames,unsigned outputRate);
private:
    using Vec=std::array<float,3>;
    struct Buffer {
        Handle handle=0;unsigned channels=0,rate=0,frames=0;
        bool stream=false;std::vector<float> pcm;
        ~Buffer();
    };
    struct Source {
        struct Spatial {
            Vec position{},direction{},velocity{};
            float minDistance=1,maxDistance=1e9f,outerGain=0;
            int inner=360,outer=360;
        } applied;
        Handle binding=0;std::shared_ptr<Buffer> loaded;
        float gain=1,pan=0,pitch=1,minDistance=1,maxDistance=1e9f,outerGain=0;
        int inner=360,outer=360;bool relative=false,streaming=false,loop=false,playing=false;
        Vec position{},direction{},velocity{};
        double cursor=0;unsigned cacheStart=0,cacheFrames=0;std::vector<float> cache;
        // Polyphase windowed-sinc filter with generated coefficients.
        double filterStep=0;std::vector<float> filter;
    };
    std::unordered_map<Handle,Source> sources;
    std::unordered_map<Handle,std::shared_ptr<Buffer>> buffers;
    Handle next=1;int error=0;
    Vec position{},velocity{},right{{1,0,0}};
    Vec pendingPosition{},pendingVelocity{},pendingRight{{1,0,0}};
    float gain=1,metresPerUnit=1;
    Source* source(Handle);
    bool fillCache(Source&,unsigned);
    bool sample(Source&,unsigned,float&,float&);
    void prepareFilter(Source&,double);
    bool resample(Source&,double,float&,float&);
    MixerLevels levels(const Source&)const;
    void applySpatial(Source&);
};
}
