// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_THREAD_CDECL TSTransform_interpolateQuatF_impl(
        const float* first,const float* second,float position,float* output);
extern "C" void MEGUSTA_THREAD_CDECL TSThread_selectKeyframes_impl(
        float position,const megusta::dts::TSShapeSequence* sequence,int* first,int* second,float* fraction);
extern "C" megusta::dts::TSThread* MEGUSTA_THREAD_THISCALL TSThread_ctor_impl(
        megusta::dts::TSThread* self,megusta::dts::TSShapeInstance* owner);
extern "C" void MEGUSTA_THREAD_THISCALL TSThread_setSequence_impl(
        megusta::dts::TSThread* self,int sequence,float position);
extern "C" void MEGUSTA_THREAD_THISCALL TSThread_transitionToSequence_impl(
        megusta::dts::TSThread* self,int sequence,float position,float duration,unsigned char targetScale);
extern "C" unsigned char MEGUSTA_THREAD_THISCALL TSThread_isInTransition_impl(
        megusta::dts::TSThread* self);
extern "C" void MEGUSTA_THREAD_FASTCALL TSThread_animateTriggers_impl(megusta::dts::TSThread* self);
extern "C" void MEGUSTA_THREAD_THISCALL TSThread_activateTriggers_impl(
        megusta::dts::TSThread* self,float start,float end);
extern "C" long double MEGUSTA_THREAD_FASTCALL TSThread_getPos_impl(megusta::dts::TSThread* self);
extern "C" long double MEGUSTA_THREAD_FASTCALL TSThread_getTime_impl(megusta::dts::TSThread* self);
extern "C" long double MEGUSTA_THREAD_FASTCALL TSThread_getDuration_impl(megusta::dts::TSThread* self);
extern "C" void MEGUSTA_THREAD_THISCALL TSThread_advancePos_impl(megusta::dts::TSThread* self,float delta);
extern "C" void MEGUSTA_THREAD_THISCALL TSThread_advanceTime_impl(megusta::dts::TSThread* self,float delta);
extern "C" void MEGUSTA_THREAD_THISCALL TSThread_setPos_impl(megusta::dts::TSThread* self,float position);
extern "C" int MEGUSTA_THREAD_THISCALL DTS_compareThreads_impl(megusta::dts::TSThread* self,megusta::dts::TSThread* other);
extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getTime_impl(megusta::dts::TSThread* thread);
extern "C" void MEGUSTA_THREAD_CDECL TSShapeInstance_setPos_impl(megusta::dts::TSThread* thread,float position);
extern "C" unsigned char MEGUSTA_THREAD_CDECL TSShapeInstance_isInTransition_impl(megusta::dts::TSThread* thread);
extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getDuration_impl(megusta::dts::TSThread* thread);
