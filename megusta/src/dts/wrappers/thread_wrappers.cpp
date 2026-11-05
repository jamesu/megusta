#include "thread_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_THREAD_CDECL TSTransform_interpolateQuatF_impl(
        const float* first,const float* second,float position,float* output) {
    interpolateQuatF(first,second,position,output);
}

extern "C" void MEGUSTA_THREAD_CDECL TSThread_selectKeyframes_impl(
        float position,const TSShapeSequence* sequence,int* first,int* second,float* fraction) {
    selectKeyframes(position,sequence,first,second,fraction);
}

extern "C" TSThread* MEGUSTA_THREAD_THISCALL TSThread_ctor_impl(
        TSThread* self,TSShapeInstance* owner) {
    return new (self) TSThread(owner);
}

extern "C" void MEGUSTA_THREAD_THISCALL TSThread_setSequence_impl(
        TSThread* self,int sequence,float position) {
    self->setSequence(sequence,position);
}

extern "C" void MEGUSTA_THREAD_THISCALL TSThread_transitionToSequence_impl(
        TSThread* self,int sequence,float position,float duration,unsigned char targetScale) {
    self->transitionToSequence(sequence,position,duration,targetScale!=0);
}

extern "C" unsigned char MEGUSTA_THREAD_THISCALL TSThread_isInTransition_impl(
        TSThread* self) {
    return self->isInTransition() ? 1u : 0u;
}

extern "C" void MEGUSTA_THREAD_FASTCALL TSThread_animateTriggers_impl(TSThread* self) {
    self->animateTriggers();
}

extern "C" void MEGUSTA_THREAD_THISCALL TSThread_activateTriggers_impl(
        TSThread* self,float start,float end) {
    self->activateTriggers(start,end);
}

extern "C" long double MEGUSTA_THREAD_FASTCALL TSThread_getPos_impl(TSThread* self) {
    return self->getPos();
}

extern "C" long double MEGUSTA_THREAD_FASTCALL TSThread_getTime_impl(TSThread* self) {
    return self->getTime();
}

extern "C" long double MEGUSTA_THREAD_FASTCALL TSThread_getDuration_impl(TSThread* self) {
    return self->getDuration();
}

extern "C" void MEGUSTA_THREAD_THISCALL TSThread_advancePos_impl(TSThread* self,float delta) {
    self->advancePos(delta);
}

extern "C" void MEGUSTA_THREAD_THISCALL TSThread_advanceTime_impl(TSThread* self,float delta) {
    self->advanceTime(delta);
}

extern "C" void MEGUSTA_THREAD_THISCALL TSThread_setPos_impl(TSThread* self,float position) {
    self->setPos(position);
}

extern "C" int MEGUSTA_THREAD_THISCALL DTS_compareThreads_impl(TSThread* self,TSThread* other) {
    return DTS_compareThreads(self,other);
}

extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getTime_impl(TSThread* thread) {
    return thread->getTime();
}

extern "C" void MEGUSTA_THREAD_CDECL TSShapeInstance_setPos_impl(TSThread* thread,float position) {
    thread->setPos(position);
}

extern "C" unsigned char MEGUSTA_THREAD_CDECL TSShapeInstance_isInTransition_impl(TSThread* thread) {
    return thread->isInTransition() ? 1u : 0u;
}

extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getDuration_impl(TSThread* thread) {
    return thread->getDuration();
}
