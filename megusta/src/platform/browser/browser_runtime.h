// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "../sdl/sdl_runtime.h"
#ifdef __EMSCRIPTEN__
namespace megusta::platform {
class BrowserRuntime final:public SdlRuntime {
public:
    ~BrowserRuntime() override { shutdown(); }
    void start();
    void runMainLoop() override { start(); }
    void shutdown() override;
    Capabilities capabilities() const override { return {false,true,false,false,false}; }
    void sleepMicroseconds(std::uint32_t) override;
protected:
    void configureContext() override;
    void contextCreated() override;
    void contextClosing() override;
    void stopMainLoop() override;
private:
    bool loopStarted=false,glInitialized=false;
};
}
#endif
