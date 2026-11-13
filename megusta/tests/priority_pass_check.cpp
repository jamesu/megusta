#include "game/sounds.h"
#include "game/gui.h"
#include "game/driver.h"
#include "game/powerups.h"
#include "bindings/game_basic.h"
#include "bindings/game_variables.h"
#include "bindings/sound_bindings.h"
#include "bindings/gui_bindings.h"
#include "bindings/remaining_game_bindings.h"
#include "bindings/runtime.h"
#include "runtime_setup.h"
#include "platform/runtime.h"
#include "engine/resource_runtime.h"
#include "engine/resource_file.h"
#include "audio/wave.h"
#include <direct.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
bool playing[256];
unsigned nextSource=1,nextBuffer=1;
float lastGain=0;
std::uint32_t now=100;
bool mountedWavRead=false,mountedStreamRead=false;
std::uint32_t clockValue() { return now; }
void writeWave(const char* path) {
    using namespace megusta::audio::wave;
    FILE* file=std::fopen(path,"wb");
    const RiffHeader riff{0x46464952,44,0x45564157};
    const ChunkHeader formatChunk{FormatTag,16},dataChunk{DataTag,8};
    const Format format{1,1,8000,8000,1,8};
    const unsigned char samples[8]={128,129,130,131,132,133,134,135};
    if (file) {
        std::fwrite(&riff,1,sizeof(riff),file);std::fwrite(&formatChunk,1,sizeof(formatChunk),file);
        std::fwrite(&format,1,sizeof(format),file);std::fwrite(&dataChunk,1,sizeof(dataChunk),file);
        std::fwrite(samples,1,sizeof(samples),file);std::fclose(file);
    }
}
}
int checkPriorityPass() {
    using namespace megusta;
    // Mounted test fixtures exercise the same shared reader entry used by WAV
    // decoding and streaming. They stay in the staged Wine C: run.
    _mkdir("data"); _mkdir("data/sounds"); _mkdir("data/voices"); _mkdir("data/music");
    for (const char* path:{"data/sounds/probe.wav","data/voices/probe.wav","data/music/probe.wav"}) writeWave(path);
    ResourceRuntime resources; const char* noArgs[]={"priority-pass"};
    resources.initializeFromArgs(1,noArgs);
    if (!resources.mounts().add("data/","data") || !resources.mounts().add("music/","data/music")) return 19;
    const auto* previous=&audio::api();
    audio::OpenALApi mock{};
    mock.genSources=[](unsigned count,unsigned* output) { for (unsigned i=0;i<count;++i) output[i]=nextSource++; return count; };
    mock.deleteSources=[](unsigned count,const unsigned* ids) { for (unsigned i=0;i<count;++i) playing[ids[i]]=false; };
    mock.genBuffers=[](unsigned count,unsigned* output) { for (unsigned i=0;i<count;++i) output[i]=nextBuffer++; return count; };
    mock.deleteBuffers=[](unsigned,const unsigned*) {};
    mock.sourcei=[](unsigned,int,int) {};
    mock.sourcef=[](unsigned,int token,float gain) { if (token==audio::Gain) lastGain=gain; };
    mock.source3f=[](unsigned,int,float,float,float) {};
    mock.listenerf=[](int,float) {};
    mock.getSourcei=[](unsigned source,int,int* output) { *output=playing[source]; };
    mock.sourcePlay=[](unsigned source) { playing[source]=true; };
    mock.sourceStop=[](unsigned source) { playing[source]=false; };
    mock.loadWav=[](ResourceFile* file,int* format,void** data,unsigned* bytes,unsigned* rate) {
        char signature[4]{}; mountedWavRead=file && file->read(signature,1,4)==4 && std::memcmp(signature,"RIFF",4)==0;
        *format=audio::Mono8; *bytes=1; *rate=8000; *data=std::malloc(1);
    };
    mock.unloadWav=[](void* data) { std::free(data); };
    mock.bufferData=[](unsigned,int,const void*,unsigned,unsigned) {};
    mock.bufferStream=[](unsigned,ResourceFile* file) {
        char signature[4]{}; mountedStreamRead=file && file->read(signature,1,4)==4 && std::memcmp(signature,"RIFF",4)==0;
        delete file;
    };
    mock.getError=[] { return 0; };
    audio::bindApi(mock);
    AudioCategoryGains gains{0.5f,1,0.6f,0.8f,1};
    Sounds sounds(gains,clockValue);
    if (!sounds.preload("probe",7) || !mountedWavRead || sounds.preload("PROBE",7)) return 2;
    if (!sounds.playStream("probe",0) || !mountedStreamRead) return 20;
    if (!sounds.stop("probe",0)) return 21;
    sounds.collectFinishedSources();
    if (!sounds.play3d("probe",1,2,3,1,99) || !sounds.isPlaying("probe",99) || lastGain!=0.4f) return 3;
    auto* source=sounds.findSource("probe",99);
    if (!(source->flags&CachedBuffer) || !(source->flags&Loop) || source->position[2]!=3) return 4;
    sounds.setPos("probe",4,5,6,99);
    if (source->position[0]!=4 || !sounds.stop("probe",99)) return 5;
    sounds.playVoice("probe",50); auto* voice=sounds.currentVoice;
    if (!voice || voice->voiceDelayOrDeadlineMs!=150) return 6;
    playing[voice->sourceHandle]=false; now=149; sounds.updateVoiceQueue();
    if (sounds.currentVoice!=voice) return 7;
    now=150; sounds.updateVoiceQueue(); if (sounds.currentVoice || (voice->flags&QueuedVoice)) return 8;
    sounds.collectFinishedSources();
    if (sounds.activeSources) return 9;
    sounds.playVoice("probe",0);
    for (int i=0;i<4;++i) sounds.playVoice("probe",0);
    auto* dropped=sounds.voiceQueue[0];
    sounds.playVoice("probe",0);
    if (sounds.voiceQueueCount!=4 || !(dropped->flags&QueuedVoice)) return 15;
    auto* removed=sounds.voiceQueue[3];
    if (!sounds.stop("probe",0) || sounds.voiceQueueCount!=3) return 16;
    for (int i=0;i<sounds.voiceQueueCount;++i) if (sounds.voiceQueue[i]==removed) return 17;
    sounds.collectFinishedSources();
    while (sounds.activeSources) delete sounds.activeSources;
    if (sounds.currentVoice || sounds.voiceQueueCount) return 18;
    bindings::GameVariables variables;
    DriverViewport viewport{640,480,640,480,0,0,false}; variables.viewport=&viewport;
    const char* names[]={"scene","rnd","fluffRnd","driver","config","gui","console","sounds","tasks",
        "font1","font2","font3","fontRenderer","camera","objects","balls","players","putter","hole","ai","powerups"};
    // Explicit nulls exercise encoding; these are test bindings, not fake
    // initialized application objects or a claimed shipped-startup run.
    for (auto* name:names) variables.set(name,nullptr);
    variables.set("sounds",&sounds);
    guiSystem gui; guiElement element;
    element.setAttr(0,"probe"); gui.elements[0]=&element; gui.elementCount=1; gui.currentElement=&element;
    variables.set("gui",&gui);
    bindings::bindGameVariables(&variables); bindings::bindSoundServices(&sounds);
    configureGamePython(); Py_OptimizeFlag=0; Py_Initialize();
    if (!bindings::initializeBasicGameModule()) { PyErr_Print(); return 10; }
    int guards=0;
    for (auto* methods:{bindings::soundBindingMethods(),bindings::guiBindingMethods(),bindings::remainingGameMethods()})
        for (auto* method=methods;method->ml_name;++method) {
            PyObject* args=PyTuple_New(0); PyObject* result=method->ml_meth(nullptr,args); Py_DECREF(args);
            if (result || !PyErr_ExceptionMatches(PyExc_TypeError)) { Py_XDECREF(result); PyErr_Print(); return 13; }
            PyErr_Clear(); ++guards;
        }
    if (guards!=85) return 14;
    PowerUps powerUps;
    PyMethodDef* enableMethod=nullptr;
    PyMethodDef* sceneRootMethod=nullptr;
    for (auto* method=bindings::remainingGameMethods();method->ml_name;++method) {
        if (!std::strcmp(method->ml_name,"PowerUps_enable")) enableMethod=method;
        if (!std::strcmp(method->ml_name,"Scene_root_get")) sceneRootMethod=method;
    }
    PyObject* encodedPowerUps=bindings::pointerResult(&powerUps,"_PowerUps_p");
    PyObject* enableArgs=PyTuple_New(2);PyTuple_SetItem(enableArgs,0,encodedPowerUps);PyTuple_SetItem(enableArgs,1,PyInt_FromLong(1));
    PyObject* enabledResult=enableMethod->ml_meth(nullptr,enableArgs);Py_DECREF(enableArgs);
    if (!enabledResult || !powerUps.powersEnabled) { Py_XDECREF(enabledResult); return 22; }
    Py_DECREF(enabledResult);
    struct SceneRootProbe { void* root; } sceneProbe{nullptr};
    PyObject* encodedScene=bindings::pointerResult(&sceneProbe,"_Scene_p");
    PyObject* sceneArgs=PyTuple_New(1);PyTuple_SetItem(sceneArgs,0,encodedScene);
    PyObject* sceneResult=sceneRootMethod->ml_meth(nullptr,sceneArgs);Py_DECREF(sceneArgs);
    if (!sceneResult || std::strcmp(PyString_AsString(sceneResult),"NULL")) { Py_XDECREF(sceneResult); return 23; }
    Py_DECREF(sceneResult);
    char script[1600];
    std::sprintf(script,
        "import gamec\nframes = []\n"
        "assert gamec.cvar.SCRN_WIDTH == 640\n"
        "assert gamec.guiSystem_getCurrent(gamec.cvar.gui) == 'probe'\n"
        "g = gamec.guiSystem_findElement(gamec.cvar.gui, 'probe')\n"
        "assert gamec.guiElement_getAttr(g, 0) == 'probe'\n"
        "assert gamec.guiElement_getEH(g, 0) is None\n"
        "assert gamec.Sounds_preload(gamec.cvar.sounds, 'probe') == 0\n"
        "def frame():\n frames.append(gamec.cvar.VIEWPORT_WIDTH)\n"
        "for name in ['scene','rnd','fluffRnd','driver','config','gui','console','sounds','tasks','font1','font2','font3','fontRenderer','camera','objects','balls','players','putter','hole','ai','powerups','SCRN_WIDTH','SCRN_HEIGHT','VIEWPORT_WIDTH','VIEWPORT_HEIGHT']:\n"
        " value = getattr(gamec.cvar, name)\n try:\n  setattr(gamec.cvar, name, value)\n except TypeError:\n  pass\n else:\n  raise RuntimeError, 'read-only global accepted write'\n");
    if (PyRun_SimpleString(script)) { PyErr_Print(); return 11; }
    platform::HeadlessRuntime headless; headless.setInitialSize(640,480); headless.create("integration");
    headless.callbacks.frame=[](void*) { PyRun_SimpleString(const_cast<char*>("frame()\n")); };
    headless.oneFrame(); viewport.viewportWidth=800;
    if (PyRun_SimpleString(const_cast<char*>("assert frames == [640]\nassert gamec.cvar.VIEWPORT_WIDTH == 800\n"))) { PyErr_Print(); return 12; }
    Py_Finalize(); bindings::bindGameVariables(nullptr); bindings::bindSoundServices(nullptr);
    std::free(element.attributes[0]); sounds.unloadGroup(-1); audio::bindApi(*previous);
    std::puts("priority pass: sound cache/playback/gain/voice deadlines, Python globals/GUI bindings and headless script-frame integration passed");
    return 0;
}
