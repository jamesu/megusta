#pragma once
namespace megusta::audio {
using BrowserAudioRender=void (*)(float*,unsigned,unsigned);
bool openBrowserAudioOutput(BrowserAudioRender,void (*failed)());
void closeBrowserAudioOutput();
}
