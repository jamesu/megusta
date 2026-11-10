#include "portable_mixer.h"
#include "stream_decoder.h"
#include "engine/resource_file.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace megusta::audio {
int mixerGainUnits(double g){return g>=1?0:g<=1e-5?-10000:static_cast<int>(std::log10(g)*2000);}
int mixerPanUnits(double p){return p>=1?10000:p<=-1?-10000:p==0?0:static_cast<int>(p>0?-2000*std::log10(1-p):2000*std::log10(1+p));}
namespace {
constexpr double Pi=3.14159265358979323846;
constexpr double SoundSpeed=360;
constexpr int FilterTaps=32,FilterPhases=256;
// Legacy gain curve: 6 dB per amplitude halving.
double amplitude(int units){return units<=-10000?0:std::exp2(units/600.0);}
double length(const std::array<float,3>& v){return std::sqrt(double(v[0])*v[0]+double(v[1])*v[1]+double(v[2])*v[2]);}
double dot(const std::array<float,3>& a,const std::array<float,3>& b){return double(a[0])*b[0]+double(a[1])*b[1]+double(a[2])*b[2];}
}
PortableMixer::Buffer::~Buffer(){if(stream)releaseStream(handle);}
void PortableMixer::clear(){sources.clear();buffers.clear();position=pendingPosition={};velocity=pendingVelocity={};right=pendingRight={1,0,0};gain=1;metresPerUnit=1;error=0;}
void PortableMixer::applySpatial(Source& s){
    s.applied={s.position,s.direction,s.velocity,s.minDistance,s.maxDistance,s.outerGain,s.inner,s.outer};
}
PortableMixer::Source* PortableMixer::source(Handle h){auto i=sources.find(h);if(i==sources.end()){fail(InvalidOperation);return nullptr;}return &i->second;}
unsigned PortableMixer::genSources(unsigned n,Handle* h){for(unsigned i=0;i<n;++i){h[i]=next++;sources.emplace(h[i],Source{});}return n;}
unsigned PortableMixer::genBuffers(unsigned n,Handle* h){for(unsigned i=0;i<n;++i){h[i]=next++;auto b=std::make_shared<Buffer>();b->handle=h[i];buffers.emplace(h[i],std::move(b));}return n;}
void PortableMixer::deleteSources(unsigned n,const Handle* h){for(unsigned i=0;i<n;++i){if(!sources.erase(h[i]))fail(InvalidOperation);}}
void PortableMixer::deleteBuffers(unsigned n,const Handle* h){for(unsigned i=0;i<n;++i){
    auto b=buffers.find(h[i]);if(b==buffers.end()){fail(InvalidOperation);continue;}
    for(auto& entry:sources)if(entry.second.loaded==b->second && b->second->stream){entry.second.playing=false;entry.second.loaded.reset();entry.second.cache.clear();}
    buffers.erase(b);
}}
void PortableMixer::bufferData(Handle h,int format,const void* data,unsigned bytes,unsigned rate){
    if(!isBuffer(h) || !data){fail(InvalidOperation);return;}
    const unsigned type=static_cast<unsigned>(format-Mono8);
    if(type>3 || !rate){fail(InvalidValue);return;}
    auto b=std::make_shared<Buffer>();b->handle=h;b->channels=(type&2)?2:1;b->rate=rate;
    const unsigned stride=b->channels*((type&1)?2:1);b->frames=bytes/stride;
    if(!b->frames){fail(InvalidValue);return;}
    b->pcm.resize(static_cast<std::size_t>(b->frames)*b->channels);
    const auto* p=static_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<b->pcm.size();++i){
        const short value=(type&1)?static_cast<short>(unsigned(p[i*2])|(unsigned(p[i*2+1])<<8)):0;
        b->pcm[i]=(type&1)?value/32768.0f:(int(p[i])-128)/128.0f;
    }
    deleteBuffers(1,&h);buffers[h]=std::move(b);
}
void PortableMixer::bufferStream(Handle h,ResourceFile* file){
    std::unique_ptr<ResourceFile> owner(file);
    if(!isBuffer(h) || !file){fail(InvalidOperation);return;}
    deleteBuffers(1,&h);StreamInfo info{};
    auto b=std::make_shared<Buffer>();b->handle=h;buffers[h]=b;
    if(!registerStream(h,owner.release(),info)){fail(InvalidValue);return;}
    b->stream=true;b->channels=info.channels;b->rate=info.rate;b->frames=info.frames;
}
void PortableMixer::sourcei(Handle h,int t,int v){auto* s=source(h);if(!s)return;
    switch(t){
    case Looping:case Streaming:case SourceRelative:
        if(v!=0 && v!=1){fail(InvalidValue);return;}
        if(t==Looping)s->loop=v;else if(t==Streaming)s->streaming=v;else s->relative=v;return;
    case ConeInnerAngle:case ConeOuterAngle:
        if(v<0 || v>360){fail(InvalidValue);return;}if(t==ConeInnerAngle)s->inner=v;else s->outer=v;return;
    case BufferBinding:if(v && !isBuffer(v)){fail(InvalidOperation);return;}s->binding=v;return;
    default:fail(InvalidValue);
    }
}
void PortableMixer::sourcef(Handle h,int t,float v){auto* s=source(h);if(!s)return;
    if(!std::isfinite(v)){fail(InvalidValue);return;}
    switch(t){
    case Gain:case ConeOuterGain:if(v<0){fail(InvalidValue);return;}if(t==Gain)s->gain=v;else s->outerGain=v;return;
    case Pitch:if(v<0.5f || v>2){fail(InvalidValue);return;}s->pitch=v;return;
    case Pan:if(v>=-1 && v<=1)s->pan=v;return;
    case MinDistance:if(v<1 || v>=1e9f){fail(InvalidValue);return;}s->minDistance=v;return;
    case MaxDistance:if(v<=1 || v>1e9f){fail(InvalidValue);return;}s->maxDistance=v;return;
    default:fail(InvalidValue);
    }
}
void PortableMixer::source3f(Handle h,int t,float x,float y,float z){auto* s=source(h);if(!s)return;
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)){fail(InvalidValue);return;}
    if(t==Position)s->position={x,y,z};else if(t==Velocity)s->velocity={x,y,z};else if(t==Direction)s->direction={x,y,z};else fail(InvalidValue);
}
void PortableMixer::listenerf(int t,float v){
    if(t==CommitDeferred){
        position=pendingPosition;velocity=pendingVelocity;right=pendingRight;
        for(auto& entry:sources)if(entry.second.loaded)applySpatial(entry.second);
        return;
    }
    if(t!=Gain || !std::isfinite(v) || v<0){fail(InvalidValue);return;}gain=v;
}
void PortableMixer::listener3f(int t,float x,float y,float z){
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)){fail(InvalidValue);return;}
    if(t==Position)pendingPosition={x,y,z};else if(t==Velocity)pendingVelocity={x,y,z};else fail(InvalidValue);
}
void PortableMixer::listenerfv(int t,const float* v){if(!v){fail(InvalidOperation);return;}
    if(t!=Orientation){listener3f(t,v[0],v[1],v[2]);return;}
    // Game orientation is up followed by forward, in game XYZ coordinates.
    // The backend's Y/Z swap reverses handedness: DirectSound's right axis
    // mapped back to game space is forward cross up, not up cross forward.
    Vec r={v[4]*v[2]-v[5]*v[1],v[5]*v[0]-v[3]*v[2],v[3]*v[1]-v[4]*v[0]};
    const double l=length(r);if(!std::isfinite(l)||l==0){fail(InvalidValue);return;}
    for(int i=0;i<3;++i)pendingRight[i]=static_cast<float>(r[i]/l);
}
void PortableMixer::distanceScale(float v){if(!std::isfinite(v)||v<=0){fail(InvalidValue);return;}metresPerUnit=v;}
MixerLevels PortableMixer::levels(const Source& s)const {
    double attenuation=mixerGainUnits(s.gain),doppler=1,rightPower=0.5;
    if(!s.relative && !s.streaming && !(s.loaded && s.loaded->stream)){
        const auto& spatial=s.applied;
        Vec d={spatial.position[0]-position[0],spatial.position[1]-position[1],spatial.position[2]-position[2]};
        const double distance=length(d);
        const double clampedDistance=std::max<double>(spatial.minDistance,std::min<double>(spatial.maxDistance,distance));
        attenuation-=2000*std::log10(clampedDistance/spatial.minDistance);
        if(distance>0){
            // Two speakers at +/-90 degrees: angular equal-power panning.
            // Front/back symmetry follows from this two-speaker geometry.
            rightPower+=std::asin(std::clamp(dot(d,right)/distance,-1.0,1.0))/Pi;
            const Vec relativeVelocity={spatial.velocity[0]-velocity[0],spatial.velocity[1]-velocity[1],spatial.velocity[2]-velocity[2]};
            const double radial=dot(relativeVelocity,d)*metresPerUnit/distance;
            doppler=SoundSpeed/(SoundSpeed+std::clamp(radial,-SoundSpeed/2,SoundSpeed/2));
            const double dl=length(spatial.direction);
            // Equal angles disable the cone; half-angles use whole degrees.
            if(dl>0 && spatial.inner!=spatial.outer){
                const double angle=std::acos(std::clamp(-dot(d,spatial.direction)/(distance*dl),-1.0,1.0))*180/Pi;
                const double inner=spatial.inner/2,outer=std::max(spatial.inner/2+1,spatial.outer/2);
                const double fraction=angle<=inner?0:angle>=outer?1:(angle-inner)/(outer-inner);
                attenuation+=mixerGainUnits(spatial.outerGain)*fraction;
            }
        }
        const double volume=amplitude(static_cast<int>(attenuation));
        return {static_cast<float>(volume*std::sqrt(1-rightPower)),
            static_cast<float>(volume*std::sqrt(rightPower)),doppler};
    }
    const int balance=mixerPanUnits(s.pan),volume=static_cast<int>(attenuation);
    return {static_cast<float>(amplitude(volume-std::max(0,balance))),
        static_cast<float>(amplitude(volume+std::min(0,balance))),doppler};
}
bool PortableMixer::fillCache(Source& s,unsigned frame){
    // Retain enough preceding samples for FIR taps on the next output frame.
    const unsigned requested=std::max(1u,s.loaded->rate*3/16);
    const unsigned preceding=std::min(unsigned(FilterTaps*2),(requested-1)/2);
    const unsigned start=frame>preceding?frame-preceding:0;
    unsigned count=0;const auto* data=readStreamChunk(s.loaded->handle,start,requested,count);
    if(!data || !count){fail(InvalidOperation);s.playing=false;return false;}
    s.cache.assign(data,data+count*s.loaded->channels);s.cacheStart=start;s.cacheFrames=count;return true;
}
bool PortableMixer::sample(Source& s,unsigned frame,float& left,float& rightSample){
    const auto& b=*s.loaded;const float* data;
    if(b.stream){if(frame<s.cacheStart || frame-s.cacheStart>=s.cacheFrames)if(!fillCache(s,frame))return false;
        data=s.cache.data()+(frame-s.cacheStart)*b.channels;
    }else data=b.pcm.data()+static_cast<std::size_t>(frame)*b.channels;
    left=data[0];rightSample=b.channels==2?data[1]:left;return true;
}
void PortableMixer::prepareFilter(Source& s,double step){
    if(s.filterStep==step)return;
    s.filter.resize(FilterPhases*FilterTaps);
    // Preserve original samples at 1:1. A modest transition band on rate
    // conversion reduces aliasing without an audible broad tonal change.
    const double cutoff=0.94/std::max(1.0,step);
    for(int phase=0;phase<FilterPhases;++phase){
        double sum=0;
        for(int tap=0;tap<FilterTaps;++tap){
            const double x=tap-(FilterTaps/2-1)-double(phase)/FilterPhases;
            const double windowPosition=x/(FilterTaps/2);
            const double window=std::abs(windowPosition)>=1?0:
                0.42+0.5*std::cos(Pi*windowPosition)+0.08*std::cos(2*Pi*windowPosition);
            const double argument=Pi*x*cutoff;
            const double coefficient=cutoff*(std::abs(argument)<1e-12?1:std::sin(argument)/argument)*window;
            s.filter[phase*FilterTaps+tap]=static_cast<float>(coefficient);sum+=coefficient;
        }
        for(int tap=0;tap<FilterTaps;++tap)s.filter[phase*FilterTaps+tap]/=static_cast<float>(sum);
    }
    s.filterStep=step;
}
bool PortableMixer::resample(Source& s,double step,float& left,float& rightSample){
    const auto frame=static_cast<unsigned>(s.cursor);
    if(step==1 && s.cursor==frame)return sample(s,frame,left,rightSample);
    const int phase=std::min(FilterPhases-1,static_cast<int>((s.cursor-frame)*FilterPhases));
    left=rightSample=0;
    for(int tap=0;tap<FilterTaps;++tap){
        std::int64_t index=std::int64_t(frame)+tap-(FilterTaps/2-1);
        if(s.loop){index%=s.loaded->frames;if(index<0)index+=s.loaded->frames;}
        else if(index<0 || index>=s.loaded->frames)continue; // finite sample, zero outside
        float l,r;if(!sample(s,static_cast<unsigned>(index),l,r))return false;
        const float coefficient=s.filter[phase*FilterTaps+tap];
        left+=l*coefficient;rightSample+=r*coefficient;
    }
    return true;
}
void PortableMixer::play(Handle h){auto* s=source(h);if(!s)return;
    auto b=buffers.find(s->binding);if(b==buffers.end() || !b->second->frames){fail(InvalidOperation);return;}
    if(b->second->channels>1 && !s->relative && !s->streaming && !b->second->stream){fail(InvalidValue);return;}
    s->loaded=b->second;s->cursor=0;s->playing=true;s->cacheFrames=0;applySpatial(*s);
    if(s->loaded->stream)fillCache(*s,0);
}
void PortableMixer::stop(Handle h){auto* s=source(h);if(s){s->playing=false;s->cursor=0;s->cacheFrames=0;}}
void PortableMixer::getSourcei(Handle h,int t,int* value){auto* s=source(h);if(!s||!value){fail(InvalidOperation);return;}
    switch(t){
    case IsPlaying:*value=s->playing;return;case Looping:*value=s->loop;return;
    case Streaming:*value=s->streaming;return;case SourceRelative:*value=s->relative;return;
    case BufferBinding:*value=s->binding;return;case ConeInnerAngle:*value=s->inner;return;case ConeOuterAngle:*value=s->outer;return;
    case RemainingSeconds:case RemainingMilliseconds:case RemainingMicroseconds:
        if(s->playing&&s->loop){*value=0;fail(InvalidValue);return;}
        *value=s->loaded?static_cast<int>(std::ceil(std::max(0.0,s->loaded->frames-s->cursor)/s->loaded->rate/s->pitch*1e-6)):0;return;
    default:fail(InvalidValue);
    }
}
void PortableMixer::render(float* output,unsigned frames,unsigned outputRate){
    std::fill(output,output+static_cast<std::size_t>(frames)*2,0.0f);if(!outputRate)return;
    for(auto& entry:sources){auto& s=entry.second;if(!s.playing||!s.loaded)continue;const auto& b=*s.loaded;
        const auto level=levels(s);
        const double frequency=std::clamp(std::trunc(double(b.rate)*s.pitch),100.0,100000.0);
        const double step=std::clamp(std::trunc(frequency*level.doppler),100.0,100000.0)/outputRate;
        if(step!=1 || s.cursor!=std::floor(s.cursor))prepareFilter(s,step);
        for(unsigned i=0;i<frames;++i){
            if(s.cursor>=b.frames){if(s.loop)s.cursor=std::fmod(s.cursor,b.frames);else{s.playing=false;break;}}
            float l,r;if(!resample(s,step,l,r))break;
            output[i*2]+=l*level.left;output[i*2+1]+=r*level.right;s.cursor+=step;
        }
        if(!s.loop && s.cursor>=b.frames)s.playing=false;
    }
    const float master=static_cast<float>(amplitude(mixerGainUnits(gain)));
    for(std::size_t i=0;i<static_cast<std::size_t>(frames)*2;++i)output[i]=std::clamp(output[i]*master,-1.0f,1.0f);
}
}
