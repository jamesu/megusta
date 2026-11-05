#include "directsound_internal.h"
#include "wave.h"
#include <cstdlib>
namespace megusta { namespace audio {
const OpenALApi& directSoundApi() {
    static const OpenALApi table{
        ds::initialize,ds::shutdown,ds::setDiagnostic,
        [](float scale) { ds::distanceScale(ds::current(),scale); },
        [](int token,float value) { ds::listenerf(ds::current(),token,value); },
        [](int token,float x,float y,float z) { ds::listener3f(ds::current(),token,x,y,z); },
        [](int token,const float* values) { ds::listenerfv(ds::current(),token,values); },
        [](std::uint32_t count,Handle* handles) { return ds::genSources(ds::current(),count,handles); },
        [](std::uint32_t count,const Handle* handles) { ds::deleteSources(ds::current(),count,handles); },
        [](Handle handle,int token,int value) { ds::sourcei(ds::current(),handle,token,value); },
        [](Handle handle,int token,float value) { ds::sourcef(ds::current(),handle,token,value); },
        [](Handle handle,int token,float x,float y,float z) { ds::source3f(ds::current(),handle,token,x,y,z); },
        [](Handle handle,int token,int* value) { ds::getSourcei(ds::current(),handle,token,value); },
        [](Handle handle) { ds::sourcePlay(ds::current(),handle); },
        [](Handle handle) { ds::sourceStop(ds::current(),handle); },
        [](std::uint32_t count,Handle* handles) { return ds::genBuffers(ds::current(),count,handles); },
        [](std::uint32_t count,const Handle* handles) { ds::deleteBuffers(ds::current(),count,handles); },
        [](Handle handle) { return ds::isBuffer(ds::current(),handle); },
        [](Handle handle,int format,const void* data,std::uint32_t bytes,std::uint32_t rate) {
            ds::bufferData(ds::current(),handle,format,data,bytes,rate);
        },
        [](Handle handle,ResourceFile* file) { ds::bufferStream(ds::current(),handle,file); },
        []() { return ds::getError(ds::current()); },
        wave::loadData,[](void* data) { std::free(data); }
    };
    return table;
}
} }
