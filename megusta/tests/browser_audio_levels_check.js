// Exercise the actual linked backend functions with a minimal WebAudio host.
// Usage: node tests/browser_audio_levels_check.js build/wasm32-browser/megusta_game.js
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync(process.argv[2],'utf8');
const handlers={};
const timers=new Map();let nextTimer=1,streamCount=0,refills=0;
let failStart=false;
class AudioNode {
    constructor(){this.gain={value:1};this.playbackRate={value:1};}
    connect(){} disconnect(){this.disconnected=true;}
    start(when=0){if(failStart){const e=new Error('injected start failure');e.name='InvalidStateError';throw e;}this.started=true;this.when=when;}
    stop(){if(!this.started){const e=new Error('not started');e.name='InvalidStateError';throw e;}this.stopped=true;}
}
class AudioContext {
    constructor(){this.state='suspended';this.currentTime=0;this.destination=new AudioNode();}
    resume(){this.state='running';return Promise.resolve();}
    close(){this.state='closed';return Promise.resolve();}
    createBufferSource(){return new AudioNode();}
    createChannelSplitter(){return new AudioNode();}
    createChannelMerger(){return new AudioNode();}
    createGain(){return new AudioNode();}
    createBuffer(channels,frames,rate){
        const data=Array.from({length:channels},()=>new Float32Array(frames));
        return {numberOfChannels:channels,duration:frames/rate,sampleRate:rate,getChannelData:i=>data[i]};
    }
}
const host={Module:{},HEAP16:new Int16Array(16),HEAPU8:new Uint8Array(32),window:{AudioContext,addEventListener:(n,f)=>handlers[n]=f,removeEventListener:n=>delete handlers[n]},console};
host.HEAPF32=new Float32Array(20000);
host.setInterval=f=>{const id=nextTimer++;timers.set(id,f);return id;};
host.clearInterval=id=>timers.delete(id);
host._megusta_web_stream_read=(h,frame,n)=>{++refills;const b=host.Module.megustaAudio.buffers.get(h);streamCount=b?Math.min(n,b.frames-frame):0;return streamCount?4:0;};
host._megusta_web_stream_count=()=>streamCount;
vm.createContext(host);
for(const name of ['webInitialize','webCommand','webBuffer']) {
    const line=source.split('\n').find(s=>s.startsWith('function '+name+'('));
    assert(line,'linked backend function missing: '+name);vm.runInContext(line,host);
}
host.webInitialize();const a=host.Module.megustaAudio;
const h=host.webCommand(0),s=a.sources.get(h);
s.left=new AudioNode();s.right=new AudioNode();s.relative=true;
const close=(x,y)=>assert(Math.abs(x-y)<0.0002,`${x} != ${y}`);
a.update(s);close(s.left.gain.value,1);close(s.right.gain.value,1);
host.webCommand(8,h,0x1010,0.5);close(s.left.gain.value,0.5);close(s.right.gain.value,1);
host.webCommand(8,h,0x1010,-0.5);close(s.left.gain.value,1);close(s.right.gain.value,0.5);
host.webCommand(8,h,0x100b,0);close(s.left.gain.value,0);close(s.right.gain.value,0);
host.webCommand(8,h,0x100b,2);close(s.left.gain.value,1);
host.webCommand(8,h,0x1010,0);host.webCommand(8,h,0x202,0);
host.webCommand(8,h,0x1012,10);host.webCommand(8,h,0x1013,100);
host.webCommand(9,h,0x1005,0,20,0);close(s.left.gain.value,0.5);close(s.right.gain.value,0.5);
host.webCommand(9,h,0x1005,0,200,0);close(s.left.gain.value,0.1);
handlers.pointerdown();assert.equal(a.ctx.state,'running');
// PCM decoding and real graph lifecycle (using host node doubles).
const b=host.webCommand(2);host.HEAP16[1]=-32768;host.HEAP16[2]=16384;
host.webBuffer(b,0x1101,2,4,22050);
close(a.buffers.get(b).getChannelData(0)[0],-1);close(a.buffers.get(b).getChannelData(0)[1],0.5);
host.webCommand(8,h,0x100a,b);host.webCommand(7,h);
assert.equal(host.webCommand(14,h,0x210),1);assert(s.node.started);
// Velocity Doppler and distance scaling must update a playing source.
host.webCommand(9,h,0x1007,0,-100,0);
close(s.node.playbackRate.value,343.3/243.3);
host.webCommand(10,0,0x1007,0,10,0);
close(s.node.playbackRate.value,353.3/243.3);
host.webCommand(15,0,0,2);
close(s.node.playbackRate.value,363.3/143.3);
host.webCommand(9,h,0x1007,0,0,0);host.webCommand(10,0,0x1007,0,0,0);
host.webCommand(8,h,0x1004,1.5);close(s.node.playbackRate.value,1.5);
// Cone outside attenuation acts in dB in addition to distance attenuation.
host.webCommand(8,h,0x1001,90);host.webCommand(8,h,0x1002,180);
host.webCommand(8,h,0x1003,0.1);host.webCommand(9,h,0x1006,0,1,0);
close(s.left.gain.value,0.01); // distance is clamped at 100; min distance 10.
host.webCommand(8,h,0x1004,3);assert.equal(host.webCommand(12),0xa001);
assert.equal(s.pitch,1.5);assert.equal(host.webCommand(12),0);
host.webCommand(8,h,0x1008,1);assert.equal(s.node.loop,true);
const node=s.node;host.webCommand(5,h);assert(node.stopped && node.disconnected);
assert.equal(host.webCommand(14,h,0x210),0);
host.webCommand(7,h);s.node.onended();assert.equal(s.playing,false);
host.webCommand(3,b);host.webCommand(7,h);assert.equal(host.webCommand(12),0xa002);
// Streamed sources retain at most 16 chunks, refill independently and cancel
// stale callbacks before releasing their decoder/buffer.
const stream=host.webCommand(2);host.webCommand(17,stream,1,22050,220500);
host.webCommand(8,h,0x100a,stream);host.webCommand(8,h,0x1008,0);
host.webCommand(7,h);assert.equal(s.streamState.nodes.size,16);
const st=s.streamState,pump=timers.get(st.timer),first=st.nodes.values().next().value;
a.ctx.currentTime=first.when+first.buffer.duration/st.rate;first.onended();pump();
assert.equal(st.nodes.size,16);assert(st.frame>16*Math.floor(22050*3/16));
const secondHandle=host.webCommand(0);host.webCommand(8,secondHandle,0x100a,stream);host.webCommand(7,secondHandle);
assert.equal(a.sources.get(secondHandle).streamState.nodes.values().next().value.streamFrame,0);
host.webCommand(8,h,0x1004,1);assert.notEqual(s.streamState,st);
let previous=refills;pump();assert.equal(refills,previous);
host.webCommand(3,stream);assert.equal(s.playing,false);assert.equal(a.sources.get(secondHandle).playing,false);
previous=refills;pump();assert.equal(refills,previous);
const shortStream=host.webCommand(2);host.webCommand(17,shortStream,1,22050,100);
host.webCommand(8,h,0x100a,shortStream);host.webCommand(7,h);
assert.equal(s.streamState.nodes.size,1);s.streamState.nodes.values().next().value.onended();assert.equal(s.playing,false);
host.webCommand(8,h,0x1008,1);host.webCommand(7,h);assert.equal(s.streamState.nodes.size,16);
host.webCommand(8,h,0x1008,0);assert.equal(s.streamState.nodes.size,1);
const oldRead=host._megusta_web_stream_read;host.webCommand(5,h);
host._megusta_web_stream_read=()=>0;host.webCommand(7,h);assert.equal(s.playing,false);assert.equal(host.webCommand(12),0xa002);
host._megusta_web_stream_read=oldRead;
failStart=true;host.webCommand(7,h);assert.equal(s.playing,false);assert.equal(host.webCommand(12),0xa002);failStart=false;
host.webCommand(1,h);host.webCommand(13);assert.equal(host.Module.megustaAudio,undefined);assert.equal(timers.size,0);
console.log('PASS: linked WebAudio levels, PCM, Doppler, scaling, cones, pitch, errors, looping, lifecycle and gesture resume');
console.log('PASS: bounded stream refill, independent cursors, stale callbacks, pitch restart, EOF, loop changes, read failure and teardown');
