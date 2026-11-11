#include "audio_output.h"
#ifdef __EMSCRIPTEN__
extern "C" {
int megusta_browser_audio_open(megusta::audio::BrowserAudioRender,float*,unsigned,void (*)());
void megusta_browser_audio_close();
}
namespace megusta::audio {
namespace { float samples[1024*2]; }
bool openBrowserAudioOutput(BrowserAudioRender render,void (*failed)()) {
    return megusta_browser_audio_open(render,samples,1024,failed)!=0;
}
void closeBrowserAudioOutput() { megusta_browser_audio_close(); }
}
#endif
