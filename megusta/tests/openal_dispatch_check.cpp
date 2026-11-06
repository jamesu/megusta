#include "audio/openal.h"
#include "engine/resource_runtime.h"
#include <cassert>
using namespace megusta::audio;
namespace megusta { SharedIoServices* sharedIo=nullptr; }
static int calls,initialBits,lastToken,shutdownCalls;
static float lastValue;
static void initializeBackend(int bits) { ++calls;initialBits=bits; }
static void shutdownBackend() { ++shutdownCalls; }
static void diagnostic(Diagnostic) { ++calls; }
static void sourceFloat(Handle source,int token,float value) { assert(source==17);++calls;lastToken=token;lastValue=value; }
static std::uint32_t generate(std::uint32_t count,Handle* handles) { ++calls;handles[0]=17;return count ? 1 : 0; }
static void query(Handle source,int token,int* value) { assert(source==17 && token==IsPlaying);++calls;*value=1; }
static int error() { ++calls;return InvalidValue; }
int main() {
    static OpenALApi backend{};
    backend.initialize=initializeBackend;backend.shutdown=shutdownBackend;backend.setDiagnostic=diagnostic;
    backend.sourcef=sourceFloat;backend.genSources=generate;backend.getSourcei=query;backend.getError=error;
    bindApi(backend);
    enabled=true;requestedBits=16;
    Handle handle=0;
    assert(genSources(3,&handle)==1 && handle==17);
    sourcef(handle,Gain,0.25f);assert(lastToken==Gain && lastValue==0.25f);
    assert(sourceIsPlaying(handle)==1 && getError()==InvalidValue);
    enabled=false;
    const int before=calls;
    sourcef(handle,Gain,1);assert(genSources(3,&handle)==0 && getError()==0 && sourceIsPlaying(handle)==0);
    assert(calls==before);
    int argc=2;const char* argv[]={"check","-NoSound"};
    enabled=true;initialize(&argc,const_cast<char**>(argv));assert(!enabled && calls==before);
    enabled=true;argc=1;
    initializeBridge(&argc,const_cast<char**>(argv));assert(initialBits==16);
    // Registered reconstructed shutdown uses the injected backend at process exit.
    return 0;
}
