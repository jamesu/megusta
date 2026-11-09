#include "driver.h"
#include "python_resource_import.h"
#include "../engine/string_services.h"
#include "Python.h"
#include "python/runtime_setup.h"
#include "sounds.h"
#include "room_client.h"
#include "gui.h"
#include "scene_runtime.h"
#include "ssg.h"
#include "pu.h"
#include "bindings/game_basic.h"
#include "bindings/network.h"
#include "bindings/swig_runtime.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <stdexcept>
namespace megusta {
void Driver::executePython(const char* command) { PyRun_SimpleString(const_cast<char*>(command)); }
void Driver::changeScript(const char* script,DriverScriptServices& services) {
    if (strings::length(script)>0x4e) services.fatal(services.context);
    strings::copyBounded(pendingScript,script,0x4f); pendingScript[0x4f]=0;
    char* suffix=const_cast<char*>(strings::findChar(pendingScript,'.'));
    if (suffix) *suffix=0;
    services.autorun.restoreConfig(pendingScript,*this,services,services.autorunServices);
}
void Driver::reloadScript(DriverScriptServices& services) { changeScript(lastScript,services); }
void Driver::applyPendingScript(DriverScriptServices& s) {
    if (!pendingScript[0]) return;
    const bool testMode=std::getenv("MEGUSTA_TEST_LEVEL") || std::getenv("MEGUSTA_TEST_MENU");
    s.sounds.stopNonStreaming();
    networkServiceEnd();
    s.sounds.unloadGroup(2);
    auto& runtime=s.holeServices.runtime;
    runtime.tasks.scheduledTasks.clear(runtime.tasks);
    Py_Finalize(); Py_Finalize();
    gameHole.releaseResources(s.holeServices);
    s.gui.clearScreen(); if(s.client)s.client->clearUiCallbacks();
    networkServiceEnd(); puCleanUpJunk();
    ssgGetCurrentContext()->forceBasicState();
    gameSceneRoots.clear();
    bindings::swig_init_gamec_bindings(true);
    bindings::swig_init_network_bindings(true);
    configureGamePython(); Py_Initialize(); installMountedPythonImporter(); networkServiceEnd();
    if (!bindings::initializeBasicGameModule() || !bindings::initializeNetworkModule()) {
        PyErr_Print(); s.fatal(s.context); return;
    }
    if (strings::length(pendingScript)>0x4e) s.fatal(s.context);
    std::strcpy(activeScript,pendingScript); pendingScript[0]=0;
    char* argv[11]; int argc=0; char* token=strings::tokenize(activeScript,s.argumentDelimiters);
    while (token && argc<10) { argv[argc++]=token; token=strings::tokenize(nullptr,s.argumentDelimiters); }
    argv[argc]=nullptr; PySys_SetArgv(argc,argv);
    std::strcpy(lastScript,activeScript);
    char importCommand[128];
    std::sprintf(importCommand,"from %s import * ",activeScript);
    const int importResult=PyRun_SimpleString(importCommand);
    if(importResult!=0 && testMode)
        throw std::runtime_error(std::string("Level test script failed: ")+activeScript);
    if(const char* level=std::getenv("MEGUSTA_TEST_LEVEL")) {
        static bool started=false;
        if(!started && !std::strcmp(activeScript,"mainmenu")) {
            started=true;
            s.holeServices.config.writeString("Game","filename",level);
            if(std::getenv("MEGUSTA_HEAP_REPRO")) {
                s.holeServices.config.writeString("Game","numPlayers","4");
                s.holeServices.config.writeString("Player3","owner","1");
                s.holeServices.config.writeString("Game","course","0 BUM OCT PTA");
            }
            changeScript("startgme",s);
        }
    }
    if(const char* menu=std::getenv("MEGUSTA_TEST_MENU")) {
        static bool started=false;
        if(!started && !std::strcmp(activeScript,"mainmenu")){started=true;changeScript(menu,s);}
    }
    if(std::getenv("MEGUSTA_HEAP_REPRO")) {
        static int stage=0;
        if(stage==0 && !std::strcmp(activeScript,"BUM")) {
            stage=1; std::fprintf(stderr,"Heap repro: skipping BUM for four players\n");std::fflush(stderr);
            executePython("import hud\nhud.pickUpBall()\nhud.pickUpBall()\nhud.pickUpBall()\nhud.pickUpBall()\n");
        } else if(stage==1 && !std::strcmp(activeScript,"result")) {
            stage=2; executePython("nextHole()\n");
        } else if(stage==2 && !std::strcmp(activeScript,"OCT")) {
            stage=3; std::fprintf(stderr,"Heap repro: HUD restart OCT\n");std::fflush(stderr);
            executePython("import hud\nhud.restartHole()\n");
        }
    }
}
}
