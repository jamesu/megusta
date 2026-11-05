#include "thread.h"
#include "shape.h"
#include "shape_instance.h"
#include <cmath>
#include <new>
namespace megusta { namespace dts {
void interpolateQuatF(const float* first,const float* second,float position,float* output) {
    float a[4]={first[0],first[1],first[2],first[3]};
    if (second[0]*a[0]+second[1]*a[1]+second[2]*a[2]+second[3]*a[3]<0)
        for (float& value:a) value=-value;
    float v[4]; for (int i=0;i<4;++i) v[i]=(second[i]-a[i])*position+a[i];
    float squared=v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+v[3]*v[3];
    float scale=squared>=0.857 ? (squared*0.454012-1.403517)*squared+1.949542 : (squared*0.699368-1.819985)*squared+2.126369;
    for (int i=0;i<4;++i) output[i]=scale*v[i];
}
void selectKeyframes(float position,const TSShapeSequence* sequencePointer,int* first,int* second,float* fraction) {
    const TSShapeSequence& sequence=*sequencePointer;
    if (!(sequence.flags&0x10u) && position==1.0f) {
        if (fraction) *fraction=0;
        if (first) *first=sequence.numKeyframes-1;
        if (second) *second=sequence.numKeyframes-1;
        return;
    }
    long double scaled=static_cast<long double>((sequence.flags&0x10u) ? sequence.numKeyframes : sequence.numKeyframes-1)*position;
    int key=static_cast<int>(scaled);
    if (fraction) *fraction=static_cast<float>(scaled-key);
    if (first) *first=key;
    if (second) *second=((sequence.flags&0x10u) && key==sequence.numKeyframes-1) ? 0 : key+1;
}
TSThread::TSThread(TSShapeInstance* owner) : instance(owner) { setSequence(0,0.0f); }
void TSThread::setSequence(int index,float position) {
    instance->clearTransition(this);
    sequence=&instance->shape->sequences[index]; priority=sequence->priority; pos=position;
    makePath=(sequence->flags&0x20u)!=0;
    if (position>0.9999 && (sequence->flags&0x10u)) pos=0.9999f;
    selectKeyframes(pos,sequence,&keyNum1,&keyNum2,&keyPos);
}
long double TSThread::getPos() const {
    return transitionInProgress ? static_cast<long double>(transitionPosition)
                               : static_cast<long double>(pos);
}
void TSThread::transitionToSequence(int index,float position,float duration,bool targetScale) {
    if (!transitionInProgress) {
        transitionOldRotationNodes.copy(sequence->rotationMatters);
        transitionOldTranslationNodes.copy(sequence->translationMatters);
        transitionOldScaleNodes.copy(sequence->scaleMatters);
    } else {
        transitionOldRotationNodes.overlap(sequence->rotationMatters);
        transitionOldTranslationNodes.overlap(sequence->translationMatters);
        transitionOldScaleNodes.overlap(sequence->scaleMatters);
    }
    transitionDuration=duration;
    transitionOldSequence=static_cast<int>(sequence-instance->shape->sequences.data());
    transitionOldPosition=pos; transitionPosition=0.0f;
    transitionDirection=timeScale<=0.0f ? -1.0f : 1.0f;
    transitionTargetScale=targetScale ? 1.0f : 0.0f;
    transitionInProgress=true;
    sequence=&instance->shape->sequences[index]; priority=sequence->priority; pos=position;
    makePath=(sequence->flags&0x20u)!=0;
    if (position>0.9999f && (sequence->flags&0x10u)) pos=0.9999f;
    selectKeyframes(pos,sequence,&keyNum1,&keyNum2,&keyPos);
}
bool TSThread::isInTransition() const { return transitionInProgress; }
long double TSThread::getTime() const {
    return transitionInProgress
        ? static_cast<long double>(transitionPosition)*transitionDuration
        : static_cast<long double>(sequence->duration)*pos;
}
long double TSThread::getDuration() const {
    return transitionInProgress ? static_cast<long double>(transitionDuration)
                               : static_cast<long double>(sequence->duration);
}
void TSThread::advancePos(float delta) {
    if (std::fabs(delta)>1e-5) instance->setDirty(sequence->dirtyFlags | static_cast<std::uint32_t>(transitionInProgress));
    if (transitionInProgress) {
        transitionPosition+=delta*transitionDirection;
        if (transitionPosition<0.0f || transitionPosition>=1.0f) {
            const bool restoreOld=transitionPosition<0.0f;
            const int oldSequence=transitionOldSequence;
            const float oldPosition=transitionOldPosition;
            instance->clearTransition(this);
            if (restoreOld) instance->setSequence(this,oldSequence,oldPosition);
        }
        delta=(transitionTargetScale*transitionDuration/sequence->duration)*delta;
    }
    float value=pos+delta;
    if (makePath) transitionStart=pos;
    if (!(sequence->flags&0x10u)) {
        pos=value<0 ? 0 : value>1 ? 1 : value;
        if (makePath) transitionLoop=0;
    } else {
        int whole=static_cast<int>(value);
        if (makePath) { transitionLoop=whole; if (value<0) --transitionLoop; whole=transitionLoop; }
        pos=value-static_cast<float>(whole);
        if (pos<0) pos+=1;
        if (pos>=1) pos-=1;
    }
    if (makePath) { transitionEnd=pos; animateTriggers(); }
    selectKeyframes(pos,sequence,&keyNum1,&keyNum2,&keyPos);
}
void TSThread::advanceTime(float delta) {
    const float scaledDelta=delta*timeScale;
    advancePos(static_cast<float>(static_cast<long double>(scaledDelta)/getDuration()));
}
void TSThread::setPos(float position) {
    advancePos(static_cast<float>(static_cast<long double>(position)-getPos()));
}
void TSThread::activateTriggers(float start,float end) {
    int first=sequence->firstTrigger,last=first+sequence->numTriggers,atStart=last,atEnd=last;
    float previous=-1;
    for (int i=first;i<last;++i) {
        float position=instance->shape->triggers[i].position;
        if (previous<start && start<=position) atStart=i;
        if (previous<end && end<=position) atEnd=i;
        previous=position;
    }
    if (atEnd<atStart) {
        while (--atStart>=atEnd) {
            auto state=instance->shape->triggers[atStart].state;
            bool enabled=(state&0x80000000u)!=0;
            if (state&0x40000000u) enabled=!enabled;
            instance->setTriggerStateBit(state&0x1fu,enabled);
        }
    } else while (atStart<atEnd) {
        auto state=instance->shape->triggers[atStart++].state;
        instance->setTriggerStateBit(state&0x1fu,(state>>31)!=0);
    }
}
void TSThread::animateTriggers() {
    if (!sequence->numTriggers) return;
    if (!transitionLoop) activateTriggers(transitionStart,transitionEnd);
    else if (transitionLoop==1) { activateTriggers(transitionStart,1); activateTriggers(0,transitionEnd); }
    else if (transitionLoop>1) { activateTriggers(transitionEnd,1); activateTriggers(0,transitionEnd); }
    else { activateTriggers(transitionLoop==-1 ? transitionStart : transitionEnd,0); activateTriggers(1,transitionEnd); }
}
extern "C" int MEGUSTA_THREAD_THISCALL DTS_compareThreads(TSThread* self,TSThread* other) {
    const bool selfBlend=(self->sequence->flags&8u)!=0;
    const bool otherBlend=(other->sequence->flags&8u)!=0;
    if (selfBlend!=otherBlend) return selfBlend ? 1 : -1;
    if (other->priority<self->priority) return -1;
    if (self->priority<other->priority) return 1;
    return 0;
}
extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getTime(TSThread* thread) {
    return static_cast<long double>(thread->getTime());
}
extern "C" void MEGUSTA_THREAD_CDECL TSShapeInstance_setPos(TSThread* thread,float position) {
    thread->setPos(position);
}
extern "C" unsigned char MEGUSTA_THREAD_CDECL TSShapeInstance_isInTransition(TSThread* thread) {
    return thread->isInTransition() ? 1u : 0u;
}
extern "C" long double MEGUSTA_THREAD_CDECL TSShapeInstance_getDuration(TSThread* thread) {
    return static_cast<long double>(thread->getDuration());
}
} }
