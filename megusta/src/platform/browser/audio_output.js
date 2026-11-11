/* Browser output only: the C++ PortableMixer retains all gain, spatial,
   resampling and streaming logic. No shared WASM memory enters the worklet. */
addToLibrary({
  $MegustaAudio: {
    current: null,
    processorSource: `
      class MegustaAudioProcessor extends AudioWorkletProcessor {
        constructor() {
          super(); this.blocks = []; this.offset = 0; this.active = true;
          this.port.onmessage = event => {
            if (event.data === 'stop') { this.active = false; this.blocks = []; }
            else if (this.active) this.blocks.push(event.data);
          };
        }
        process(inputs, outputs) {
          const channels = outputs[0];
          for (const channel of channels) channel.fill(0);
          if (!this.active) return false;
          if (!channels.length) return true;
          for (let frame = 0; frame < channels[0].length; ++frame) {
            if (!this.blocks.length) break;
            const block = this.blocks[0];
            channels[0][frame] = block[this.offset++];
            if (channels.length > 1) channels[1][frame] = block[this.offset++];
            else ++this.offset;
            if (this.offset === block.length) {
              this.blocks.shift(); this.offset = 0;
              this.port.postMessage(block.length / 2);
            }
          }
          return true;
        }
      }
      registerProcessor('megusta-pcm', MegustaAudioProcessor);
    `,
    close: function() {
      const state = MegustaAudio.current;
      if (!state) return;
      MegustaAudio.current = null;
      clearInterval(state.timer);
      for (const type of ['pointerdown', 'touchend', 'keydown'])
        document.removeEventListener(type, state.resume, true);
      if (state.node) {
        state.node.port.onmessage = null;
        state.node.port.postMessage('stop');
        state.node.disconnect();
      }
      state.context.close().catch(() => {});
    },
    pump: function(state) {
      if (MegustaAudio.current !== state || !state.node || state.context.state !== 'running') return;
      /* Bounded lookahead, no catch-up loops after a main-thread stall.
         The worklet acknowledges each block exactly once when consumed. */
      while (state.pendingFrames < state.frames * 3) {
        getWasmTableEntry(state.render)(state.buffer, state.frames, state.context.sampleRate);
        const block = new Float32Array(HEAPF32.subarray(state.buffer / 4,
          state.buffer / 4 + state.frames * 2));
        state.node.port.postMessage(block, [block.buffer]);
        state.pendingFrames += state.frames;
      }
    },
    open: function(render, buffer, frames, failed) {
      MegustaAudio.close();
      let context;
      try {
        context = new AudioContext();
        if (!context.audioWorklet) throw new Error('AudioWorklet requires a secure browser context');
      } catch (error) {
        if (context) context.close().catch(() => {});
        console.error('Browser audio unavailable:', error);
        return 0;
      }
      const state = {context: context, render: render, buffer: buffer, frames: frames,
        pendingFrames: 0, node: null, timer: null};
      MegustaAudio.current = state;
      state.resume = () => {
        if (MegustaAudio.current === state && context.state === 'suspended')
          context.resume().catch(error => console.warn('Browser audio resume failed:', error));
      };
      for (const type of ['pointerdown', 'touchend', 'keydown'])
        document.addEventListener(type, state.resume, true);
      const url = URL.createObjectURL(new Blob([MegustaAudio.processorSource], {type: 'text/javascript'}));
      context.audioWorklet.addModule(url).then(() => {
        URL.revokeObjectURL(url);
        if (MegustaAudio.current !== state) return;
        state.node = new AudioWorkletNode(context, 'megusta-pcm',
          {numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2]});
        state.node.port.onmessage = event => {
          if (MegustaAudio.current !== state) return;
          state.pendingFrames -= event.data;
          MegustaAudio.pump(state);
        };
        state.node.onprocessorerror = () => {
          if (MegustaAudio.current !== state) return;
          console.error('Browser audio worklet failed');
          MegustaAudio.close(); getWasmTableEntry(failed)();
        };
        state.node.connect(context.destination);
        state.timer = setInterval(() => MegustaAudio.pump(state), 20);
        MegustaAudio.pump(state);
      }).catch(error => {
        URL.revokeObjectURL(url);
        if (MegustaAudio.current !== state) return;
        console.error('Browser audio initialization failed:', error);
        MegustaAudio.close(); getWasmTableEntry(failed)();
      });
      return 1;
    }
  },
  megusta_browser_audio_open__deps: ['$MegustaAudio', '$getWasmTableEntry'],
  megusta_browser_audio_open: function(render, buffer, frames, failed) {
    return MegustaAudio.open(render, buffer, frames, failed);
  },
  megusta_browser_audio_close__deps: ['$MegustaAudio'],
  megusta_browser_audio_close: function() { MegustaAudio.close(); }
});
