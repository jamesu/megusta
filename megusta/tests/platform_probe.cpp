#include "platform/runtime.h"
#if defined(__EMSCRIPTEN__) && defined(MEGUSTA_BROWSER)
#include "platform/browser.h"
int main() {
    static megusta::platform::BrowserRuntime runtime;
    runtime.setInitialSize(640,480);
    if (!runtime.create("Megusta platform probe")) return 1;
    runtime.start(); return 0;
}
#else
int checkPlatformRuntime();
int main() { return checkPlatformRuntime(); }
#endif
