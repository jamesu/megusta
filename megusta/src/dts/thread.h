// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "sequence.h"
#if defined(_MSC_VER) && !defined(__clang__)
// These C-linkage adapters take `self` explicitly. MSVC cannot mark a free
// function __thiscall; keep the declaration/call convention consistent.
// ClangCL accepts the GNU thiscall attribute and preserves the x86 ABI.
#if defined(_M_IX86)
#define MEGUSTA_THREAD_THISCALL
#define MEGUSTA_THREAD_CDECL __cdecl
#define MEGUSTA_THREAD_FASTCALL __fastcall
#else
#define MEGUSTA_THREAD_THISCALL
#define MEGUSTA_THREAD_CDECL
#define MEGUSTA_THREAD_FASTCALL
#endif
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define MEGUSTA_THREAD_THISCALL __attribute__((thiscall))
#define MEGUSTA_THREAD_CDECL __attribute__((cdecl))
#define MEGUSTA_THREAD_FASTCALL __attribute__((fastcall))
#else
#define MEGUSTA_THREAD_THISCALL
#define MEGUSTA_THREAD_CDECL
#define MEGUSTA_THREAD_FASTCALL
#endif
namespace megusta { namespace dts {
class TSShapeInstance;
// 004ced10: selects adjacent animation keys for a normalized sequence position.
void selectKeyframes(float position,const TSShapeSequence* sequence,int* first,int* second,float* fraction);
void interpolateQuatF(const float* first,const float* second,float position,float* output);
// Normalized animation thread with sequence position, trigger, and transition
// state; ownership follows the instance while field layout stays portable.
class TSThread {
public:
    TSShapeInstance* instance;
    TSShapeSequence* sequence=nullptr;
    int priority=0,keyNum1=0,keyNum2=0,transitionLoop=0;
    float pos=0,keyPos=0,timeScale=1,transitionStart=0,transitionEnd=0;
    bool transitionInProgress=false;
    int transitionOldSequence=-1;
    float transitionPosition=0,transitionDuration=0,transitionDirection=1,transitionTargetScale=0,transitionOldPosition=0;
    TSIntegerSet transitionOldRotationNodes,transitionOldTranslationNodes,transitionOldScaleNodes;
    bool makePath=false,blendDisabled=false;
    explicit TSThread(TSShapeInstance* owner);
    void setSequence(int index,float position);
    void transitionToSequence(int index,float position,float duration,bool targetScale);
    bool isInTransition() const;
    void advancePos(float delta);
    void advanceTime(float delta);
    void setPos(float position);
    long double getPos() const;
    long double getTime() const;
    long double getDuration() const;
    void activateTriggers(float start,float end);
    void animateTriggers();
};
extern "C" int MEGUSTA_THREAD_THISCALL DTS_compareThreads(TSThread* self,TSThread* other);
extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getTime(TSThread* thread);
extern "C" void MEGUSTA_THREAD_CDECL TSShapeInstance_setPos(TSThread* thread,float position);
extern "C" unsigned char MEGUSTA_THREAD_CDECL TSShapeInstance_isInTransition(TSThread* thread);
extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getDuration(TSThread* thread);
} }
