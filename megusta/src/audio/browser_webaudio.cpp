#include "openal.h"
#include "wave.h"
#include "browser_audio_stream.h"
#include "engine/resource_file.h"
#include <emscripten.h>
#include <cstdlib>
namespace megusta::audio {
// WebAudio supplies sample scheduling/resampling and summation. Historical
// source state and explicit DirectSound-style channel levels live here, rather
// than in OpenAL's distance model or WebAudio's equal-power StereoPannerNode.
EM_JS(int, webInitialize, (), {
    if(Module.megustaAudio)return 1;
    const C=window.AudioContext || window.webkitAudioContext;
    if(!C)return 0;
    let ctx;try{ctx=new C({sampleRate:22050});}catch(e){try{ctx=new C();}catch(f){console.error('WebAudio initialization failed',f);return 0;}}
    const a=Module.megustaAudio={ctx:ctx,sources:new Map(),buffers:new Map(),next:1,
        listener:[0,0,0],velocity:[0,0,0],right:[-1,0,0],gain:1,distanceScale:1,error:0};
    a.fail=function(error){if(!a.error)a.error=error;return 0;};
    a.cursor=function(s){
        if(s.streamState){
            const st=s.streamState;let frame=st.completedFrame;
            for(const n of st.nodes){if(ctx.currentTime<n.streamTime)break;frame=n.streamFrame+Math.min(n.streamFrames,Math.max(0,ctx.currentTime-n.streamTime)*st.rate*st.buffer.sampleRate);if(frame<n.streamFrame+n.streamFrames)break;}
            s.offset=frame/st.buffer.sampleRate;return s.offset;
        }
        if(s.playing){s.offset+=Math.max(0,ctx.currentTime-s.clock)*s.rate;s.clock=Math.max(s.clock,ctx.currentTime);}return s.offset;
    };
    a.level=function(g){return g>=1?1:g<=0.00001?0:Math.pow(10,Math.trunc(Math.log10(g)*2000)/2000);};
    a.panLevel=function(p){return p>=1?0:p<=0?1:Math.pow(10,-Math.trunc(-Math.log10(1-p)*2000)/2000);};
    a.update=function(s){
        if(!s.left)return;
        let gain=a.level(s.gain)*a.level(a.gain),pan=s.pan,doppler=1;
        if(!s.relative && !s.streaming){
            const d=s.position.map(function(v,i){return v-a.listener[i];});
            const distance=Math.hypot(d[0],d[1],d[2]);
            gain*=s.min/Math.max(s.min,Math.min(s.max,distance));
            if(distance>0)pan=Math.max(-1,Math.min(1,d.reduce(function(sum,v,i){return sum+v*a.right[i];},0)/distance));
            if(distance>0){
                const speed=343.3;
                const radial=function(v){return Math.max(-speed*0.99,Math.min(speed*0.99,v.reduce(function(sum,value,i){return sum+value*d[i];},0)*a.distanceScale/distance));};
                doppler=(speed+radial(a.velocity))/(speed+radial(s.velocity));
            }
            const directionLength=Math.hypot.apply(null,s.direction);
            if(distance>0 && directionLength>0 && s.outer<360){
                const dot=-d.reduce(function(sum,v,i){return sum+v*s.direction[i];},0)/(distance*directionLength);
                const angle=Math.acos(Math.max(-1,Math.min(1,dot)))*180/Math.PI;
                const inner=s.inner/2,outer=s.outer/2;
                const t=angle<=inner?0:angle>=outer?1:(angle-inner)/(outer-inner);
                gain*=Math.pow(10,Math.trunc((Math.log10(Math.max(0.00001,Math.min(1,s.outerGain)))*2000)*t)/2000);
            }
        }
        s.left.gain.value=gain*a.panLevel(Math.max(0,pan));
        s.right.gain.value=gain*a.panLevel(Math.max(0,-pan));
        if(s.node){
            a.cursor(s);
            const frequency=Math.max(100,Math.min(100000,Math.trunc(s.node.buffer.sampleRate*s.pitch)));
            s.rate=Math.max(100,Math.min(100000,frequency*doppler))/s.node.buffer.sampleRate;
            s.node.loop=!!s.loop;s.node.playbackRate.value=s.rate;
        }
        if(s.streamState){
            const st=s.streamState;
            const rate=Math.max(100,Math.min(100000,Math.trunc(st.buffer.sampleRate*s.pitch)))/st.buffer.sampleRate;
            if(rate!==st.rate || !!s.loop!==st.loop){
                const frame=Math.floor(a.cursor(s)*st.buffer.sampleRate);
                a.startStream(s,st.buffer,st.loop?frame%st.buffer.frames:frame);
            }
        }
    };
    a.disposeNode=function(n){n.onended=null;try{n.stop();}catch(e){if(e.name!=='InvalidStateError')console.warn('Audio node stop failed',e);}n.disconnect();};
    a.stop=function(s){
        if(s.streamState){const st=s.streamState;s.streamState=null;clearInterval(st.timer);st.nodes.forEach(a.disposeNode);st.nodes.clear();}
        if(s.node){a.disposeNode(s.node);s.node=null;}if(s.split)s.split.disconnect();if(s.left)s.left.disconnect();if(s.right)s.right.disconnect();if(s.merge)s.merge.disconnect();s.playing=false;
    };
    a.startStream=function(s,buffer,frame){
        try{
        a.stop(s);
        if(frame>=buffer.frames){s.offset=buffer.duration;return;}
        s.split=ctx.createChannelSplitter(buffer.numberOfChannels);
        s.left=ctx.createGain();s.right=ctx.createGain();s.merge=ctx.createChannelMerger(2);
        s.split.connect(s.left,0);s.split.connect(s.right,buffer.numberOfChannels>1?1:0);
        s.left.connect(s.merge,0,0);s.right.connect(s.merge,0,1);s.merge.connect(ctx.destination);
        const st={buffer:buffer,frame:frame,completedFrame:frame,nextTime:ctx.currentTime+0.02,nodes:new Set(),eof:false,
            rate:Math.max(100,Math.min(100000,Math.trunc(buffer.sampleRate*s.pitch)))/buffer.sampleRate,loop:!!s.loop,timer:null};
        s.streamState=st;s.rate=st.rate;s.offset=frame/buffer.sampleRate;s.clock=st.nextTime;s.playing=true;a.update(s);
        const pump=function(){
            if(Module.megustaAudio!==a || s.streamState!==st)return;
            try{
                while(st.nodes.size<16 && !st.eof){
                    if(st.frame>=buffer.frames){if(st.loop)st.frame=0;else {st.eof=true;break;}}
                    const ptr=_megusta_web_stream_read(buffer.handle,st.frame,Math.max(1,Math.floor(buffer.sampleRate*3/16)));
                    const count=_megusta_web_stream_count();
                    if(!ptr || !count){a.fail(0xa002);a.stop(s);return;}
                    const chunk=ctx.createBuffer(buffer.numberOfChannels,count,buffer.sampleRate);
                    // Fetch HEAPF32 after the call: a refill can grow WASM memory.
                    for(let c=0;c<buffer.numberOfChannels;++c){const out=chunk.getChannelData(c);for(let i=0;i<count;++i)out[i]=HEAPF32[(ptr>>2)+i*buffer.numberOfChannels+c];}
                    const node=ctx.createBufferSource();node.buffer=chunk;node.playbackRate.value=st.rate;node.connect(s.split);
                    st.nextTime=Math.max(st.nextTime,ctx.currentTime+0.02);
                    node.streamFrame=st.frame;node.streamFrames=count;node.streamTime=st.nextTime;
                    st.nodes.add(node);node.onended=function(){
                        node.disconnect();st.nodes.delete(node);
                        if(s.streamState===st)st.completedFrame=node.streamFrame+node.streamFrames;
                        if(s.streamState===st && st.eof && !st.nodes.size){a.stop(s);s.offset=buffer.duration;}
                    };
                    node.start(st.nextTime);st.nextTime+=count/buffer.sampleRate/st.rate;st.frame+=count;
                }
            }catch(e){a.fail(0xa002);a.stop(s);console.error('Audio stream refill failed',e);}
        };
        st.timer=setInterval(pump,25);pump();
        }catch(e){a.fail(0xa002);a.stop(s);console.error('Audio stream setup failed',e);}
    };
    a.unlock=function(){if(ctx.state==='suspended')ctx.resume().catch(function(e){console.warn('Audio resume failed',e);});};
    window.addEventListener('pointerdown',a.unlock,true);window.addEventListener('keydown',a.unlock,true);
    return 1;
});
EM_JS(int, webCommand, (int op,int handle,int name,double x,double y,double z), {
    const a=Module.megustaAudio;if(!a)return 0;
    let s=a.sources.get(handle);
    if([9,10,11].includes(op) && ![x,y,z].every(Number.isFinite))return a.fail(0xa001);
    if(op===0){const h=a.next++;a.sources.set(h,{gain:1,pan:0,pitch:1,relative:false,streaming:false,loop:false,position:[0,0,0],velocity:[0,0,0],direction:[0,0,0],min:1,max:1e9,inner:360,outer:360,outerGain:0,buffer:0,playing:false,offset:0,clock:0,rate:1});return h;}
    if([1,5,6,7,8,9,14].includes(op) && !s)return a.fail(0xa002);
    if(op===1){if(s){a.stop(s);a.sources.delete(handle);}return 0;}
    if(op===2){const h=a.next++;a.buffers.set(h,null);return h;}
    if(op===3){if(!a.buffers.has(handle))return a.fail(0xa002);a.sources.forEach(function(source){if(source.streamState && source.streamState.buffer.handle===handle)a.stop(source);});a.buffers.delete(handle);return 0;}
    if(op===4)return a.buffers.has(handle)?1:0;
    if(op===5){a.stop(s);s.offset=0;return 0;}
    if(op===6)return s && s.playing?1:0;
    if(op===7){
        if(!a.buffers.get(s.buffer))return a.fail(0xa002);
        a.stop(s);const buffer=a.buffers.get(s.buffer),ctx=a.ctx;
        if(buffer.stream){a.startStream(s,buffer,0);return 0;}
        if(!s.relative && !s.streaming && buffer.numberOfChannels>1)return a.fail(0xa001);
        s.node=ctx.createBufferSource();s.node.buffer=buffer;
        s.split=ctx.createChannelSplitter(buffer.numberOfChannels);
        s.left=ctx.createGain();s.right=ctx.createGain();s.merge=ctx.createChannelMerger(2);
        s.node.connect(s.split);s.split.connect(s.left,0);s.split.connect(s.right,buffer.numberOfChannels>1?1:0);
        s.left.connect(s.merge,0,0);s.right.connect(s.merge,0,1);s.merge.connect(ctx.destination);
        s.offset=0;s.clock=ctx.currentTime;
        a.update(s);s.playing=true;s.node.onended=function(){a.stop(s);s.offset=buffer.duration;};s.node.start();return 0;
    }
    if(op===8 && s){
        const keys={514:'relative',4097:'inner',4098:'outer',4099:'outerGain',4100:'pitch',4104:'loop',4105:'streaming',4106:'buffer',4107:'gain',4112:'pan',4114:'min',4115:'max'};
        if(name===521)return 0;
        if(!keys[name] || !Number.isFinite(x))return a.fail(0xa001);
        if([514,4104,4105].includes(name) && x!==0 && x!==1)return a.fail(0xa001);
        if([4097,4098].includes(name) && (x<0 || x>360))return a.fail(0xa001);
        if([4099,4107].includes(name) && x<0)return a.fail(0xa001);
        if(name===4100 && (x<0.5 || x>2))return a.fail(0xa001);
        if(name===4112 && (x< -1 || x>1))return 0;
        if(name===4114 && (x<1 || x>=1e9))return a.fail(0xa001);
        if(name===4115 && (x<=1 || x>1e9))return a.fail(0xa001);
        if(name===4106 && x!==0 && !a.buffers.has(x))return a.fail(0xa002);
        s[keys[name]]=x;
        a.update(s);return 0;
    }
    if(op===9 && s){if(name===4101)s.position=[x,y,z];else if(name===4102)s.direction=[x,y,z];else if(name===4103)s.velocity=[x,y,z];else return a.fail(0xa001);a.update(s);return 0;}
    if(op===10){if(name===4101)a.listener=[x,y,z];else if(name===4103)a.velocity=[x,y,z];else if(name===4107 && x>=0)a.gain=x;else return a.fail(0xa001);a.sources.forEach(a.update);return 0;}
    if(op===11){a.right=[x,y,z];a.sources.forEach(a.update);return 0;}
    if(op===12){const e=a.error;a.error=0;return e;}
    if(op===14){
        const keys={4097:'inner',4098:'outer',4104:'loop',4105:'streaming',4106:'buffer'};
        if(keys[name])return s[keys[name]];
        if(name===528)return s.playing?1:0;
        if(name===514)return s.relative?1:0;
        if(name>=518 && name<=520){
            if(s.playing && s.loop)return a.fail(0xa001);
            const buffer=s.streamState?s.streamState.buffer:s.node?s.node.buffer:a.buffers.get(s.buffer);
            if(!buffer)return 0;
            const remaining=Math.max(0,buffer.duration-a.cursor(s))/s.rate;
            return Math.ceil(remaining*0.000001);
        }
        return a.fail(0xa001);
    }
    if(op===15){if(x<=0 || !Number.isFinite(x))return a.fail(0xa002);a.distanceScale=x;a.sources.forEach(a.update);return 0;}
    if(op===13){a.sources.forEach(a.stop);window.removeEventListener('pointerdown',a.unlock,true);window.removeEventListener('keydown',a.unlock,true);a.ctx.close().catch(function(e){console.warn('Audio close failed',e);});delete Module.megustaAudio;}
    if(op===16)return a.fail(name);
    if(op===17){if(!a.buffers.has(handle))return a.fail(0xa002);a.buffers.set(handle,{stream:true,handle:handle,numberOfChannels:name,sampleRate:x,frames:y,duration:y/x});return 0;}
    if(op===18){if(!a.buffers.has(handle))return a.fail(0xa002);a.sources.forEach(function(source){if(source.streamState && source.streamState.buffer.handle===handle)a.stop(source);});a.buffers.set(handle,null);return 0;}
    return 0;
});
EM_JS(void, webBuffer, (int handle,int format,const void* data,unsigned bytes,unsigned rate), {
    const a=Module.megustaAudio,index=format-0x1100;
    if(!a)return;
    if(!a.buffers.has(handle) || !data){a.fail(0xa002);return;}
    if(index<0 || index>3 || !rate){a.fail(0xa001);return;}
    const channels=index>=2?2:1,bits=(index&1)?16:8;
    const frames=Math.floor(bytes/(channels*(bits/8)));
    if(!frames){a.fail(0xa001);return;}
    let buffer;try{buffer=a.ctx.createBuffer(channels,frames,rate);}catch(e){a.fail(e.name==='NotSupportedError'?0xa001:0xa003);return;}
    for(let c=0;c<channels;++c){const out=buffer.getChannelData(c);for(let i=0;i<frames;++i){const p=i*channels+c;out[i]=bits===16?HEAP16[(data>>1)+p]/32768:(HEAPU8[data+p]-128)/128;}}
    a.buffers.set(handle,buffer);
});
namespace {
Diagnostic diagnostic=nullptr;
int command(int op,Handle h=0,int token=0,double x=0,double y=0,double z=0){return webCommand(op,h,token,x,y,z);}
void streamBuffer(Handle h,ResourceFile* file){
    if(!file){command(16,0,InvalidOperation);return;}
    if(!command(4,h)){delete file;command(16,0,InvalidOperation);return;}
    command(18,h);releaseBrowserStream(h);
    BrowserStreamInfo info{};
    if(registerBrowserStream(h,file,info))command(17,h,info.channels,info.rate,info.frames);
    else command(16,0,InvalidValue);
}
}
const OpenALApi& browserWebAudioApi(){
    static const OpenALApi table{
        [](int){if(!webInitialize()){enabled=false;if(diagnostic)diagnostic("Unable to initialize WebAudio");}},[](){command(13);releaseBrowserStreams();},[](Diagnostic callback){diagnostic=callback;},[](float scale){command(15,0,0,scale);},
        [](int t,float v){if(t==Gain)command(10,0,t,v);else if(t!=CommitDeferred)command(16,0,InvalidValue);},
        [](int t,float x,float y,float z){command(10,0,t,x,y,z);},
        [](int t,const float* v){if(!v){command(16,0,InvalidOperation);return;}if(t==Orientation){const float x=v[1]*v[5]-v[2]*v[4],y=v[2]*v[3]-v[0]*v[5],z=v[0]*v[4]-v[1]*v[3];const float length=__builtin_sqrtf(x*x+y*y+z*z);if(length>0)command(11,0,0,x/length,y/length,z/length);else command(16,0,InvalidValue);}else command(10,0,t,v[0],v[1],v[2]);},
        [](std::uint32_t n,Handle* h){for(unsigned i=0;i<n;++i)h[i]=command(0);return n;},
        [](std::uint32_t n,const Handle* h){for(unsigned i=0;i<n;++i)command(1,h[i]);},
        [](Handle h,int t,int v){command(8,h,t,v);},[](Handle h,int t,float v){command(8,h,t,v);},
        [](Handle h,int t,float x,float y,float z){command(9,h,t,x,y,z);},
        [](Handle h,int t,int* v){if(v)*v=command(14,h,t);else command(16,0,InvalidOperation);},
        [](Handle h){command(7,h);},[](Handle h){command(5,h);},
        [](std::uint32_t n,Handle* h){for(unsigned i=0;i<n;++i)h[i]=command(2);return n;},
        [](std::uint32_t n,const Handle* h){for(unsigned i=0;i<n;++i){command(3,h[i]);releaseBrowserStream(h[i]);}},
        [](Handle h){return command(4,h)!=0;},
        [](Handle h,int f,const void* p,std::uint32_t n,std::uint32_t r){command(18,h);releaseBrowserStream(h);webBuffer(h,f,p,n,r);},
        streamBuffer,[](){const int error=command(12);if(error && diagnostic)diagnostic("WebAudio backend reported an audio API error");return error;},wave::loadData,[](void* p){std::free(p);}
    };return table;
}
}
