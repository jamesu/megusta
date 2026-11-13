const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const path = require('node:path');
let library;
vm.runInNewContext(fs.readFileSync(path.join(__dirname,
  '../src/platform/browser/audio_output.js'), 'utf8'), {addToLibrary: value => library = value});
let Processor;
class BaseProcessor {
  constructor() { this.port = {postMessage: count => this.consumed.push(count)}; this.consumed = []; }
}
vm.runInNewContext(library.$MegustaAudio.processorSource,
  {AudioWorkletProcessor: BaseProcessor, registerProcessor: (name, type) => Processor = type});
const processor = new Processor();
const output = [new Float32Array(128), new Float32Array(128)];
const block = new Float32Array(1024 * 2);
for (let i = 0; i < 1024; ++i) {block[i * 2] = i / 1024; block[i * 2 + 1] = -i / 1024;}
processor.port.onmessage({data: block});
for (let chunk = 0; chunk < 8; ++chunk) {
  assert.equal(processor.process([], [output]), true);
  for (let i = 0; i < 128; ++i) {
    assert.equal(output[0][i], block[(chunk * 128 + i) * 2]);
    assert.equal(output[1][i], block[(chunk * 128 + i) * 2 + 1]);
  }
}
assert.deepEqual(processor.consumed, [1024]);
/* Simulate several seconds of map-loading starvation. Reused output buffers
   deliberately contain old samples, so omitted clearing fails this test. */
for (let i = 0; i < 2000; ++i) {
  output[0].fill(1); output[1].fill(1);
  assert.equal(processor.process([], [output]), true);
  assert.ok(output.every(channel => channel.every(sample => sample === 0)));
}
assert.deepEqual(processor.consumed, [1024]);
processor.port.onmessage({data: new Float32Array([0.25, -0.25])});
processor.process([], [output]);
assert.equal(output[0][0], 0.25); assert.equal(output[1][0], -0.25);
assert.equal(output[0][1], 0); assert.equal(output[1][127], 0);
processor.port.onmessage({data: 'stop'});
assert.equal(processor.process([], [output]), false);
assert.ok(output.every(channel => channel.every(sample => sample === 0)));

/* Exercise the real producer implementation: bounded lookahead, consumed
   credits, owned copies across WASM heap changes, and stale-session rejection. */
const heap = new Float32Array(2048);
let renders = 0;
const getEntry = () => (pointer, frames, rate) => {
  assert.equal(frames, 1024); assert.equal(rate, 48000);
  heap.fill(++renders);
};
/* Functions created by vm retain that VM's globals. */
let boundLibrary;
const context = {addToLibrary: value => boundLibrary = value, HEAPF32: heap,
  getWasmTableEntry: getEntry};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,
  '../src/platform/browser/audio_output.js'), 'utf8'), context);
context.MegustaAudio = boundLibrary.$MegustaAudio;
const producer = context.MegustaAudio;
const posted = [];
const state = {context: {state: 'running', sampleRate: 48000}, render: 1, buffer: 0,
  frames: 1024, pendingFrames: 0, node: {port: {postMessage: block => posted.push(block)}}};
producer.current = state; producer.pump(state);
assert.equal(renders, 3); assert.equal(state.pendingFrames, 3072);
heap.fill(100); assert.equal(posted[0][0], 1);
producer.pump(state); assert.equal(renders, 3);
state.pendingFrames -= 1024; producer.pump(state); assert.equal(renders, 4);
state.context.state = 'suspended'; state.pendingFrames = 0;
producer.pump(state); assert.equal(renders, 4);
state.context.state = 'running'; producer.current = null;
producer.pump(state); assert.equal(renders, 4);
console.log('PASS: consume once, silence on stalls, resume, stop, bounded queue and independent PCM ownership');
