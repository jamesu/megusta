#include "game_shape_adapter.h"
#include "dts/shape_instance.h"
#include "dts/thread.h"
namespace megusta {
const ShapeAnimationServices& nativeShapeAnimationServices() {
    static const ShapeAnimationServices services={
        [](void* instance,void* thread,int sequence,float position) { static_cast<dts::TSShapeInstance*>(instance)->setSequence(static_cast<dts::TSThread*>(thread),sequence,position); },
        [](void* instance,void* thread,int sequence,float position,float duration,bool targetScale) { static_cast<dts::TSShapeInstance*>(instance)->transitionToSequence(static_cast<dts::TSThread*>(thread),sequence,position,duration,targetScale); },
        [](void* thread) { return static_cast<dts::TSThread*>(thread)->getDuration(); },
        [](void* thread) { return static_cast<dts::TSThread*>(thread)->getTime(); },
        [](void* thread,float position) { static_cast<dts::TSThread*>(thread)->setPos(position); },
        [](void* thread) { return static_cast<dts::TSThread*>(thread)->isInTransition(); },
        [](void* instance,float time) { static_cast<dts::TSShapeInstance*>(instance)->advanceTime(time); }
    };
    return services;
}
float gameShapeMeshColour[3]={1.0f,1.0f,1.0f};
void setGameShapeMeshColour(float red,float green,float blue) {
    gameShapeMeshColour[0]=red; gameShapeMeshColour[1]=green; gameShapeMeshColour[2]=blue;
}
void GameShapeAdapter::initialize() {
    playbackFlags=0; shapeName=nullptr; shape=nullptr; instance=nullptr; thread=nullptr;
    importedSequenceCount=0; currentSequence=0; optionalShadowObject=nullptr;
    borrowedShape=false; baseSkinMaterial=0; activeSkinMaterial=0;
}
void GameShapeAdapter::setSequence(int sequence,const ShapeAnimationServices& s) {
    if (!instance || !thread || sequence==currentSequence || sequence<0 || sequence>=importedSequenceCount) return;
    if (currentSequence==-1) s.setSequence(instance,thread,sequence,0);
    else s.transitionToSequence(instance,thread,sequence,0,0.2f,true);
    currentSequence=sequence;
}
long double GameShapeAdapter::getDuration(const ShapeAnimationServices& s) const { return instance && thread ? s.duration(thread) : 0.0L; }
long double GameShapeAdapter::getTime(const ShapeAnimationServices& s) const { return instance && thread ? s.time(thread) : 0.0L; }
void GameShapeAdapter::setTime(float value,const ShapeAnimationServices& s) {
    if (instance && thread) s.setPosition(thread,static_cast<float>(static_cast<long double>(value)/s.duration(thread)));
}
void GameShapeAdapter::setPosition(float value,const ShapeAnimationServices& s) { if (instance && thread) s.setPosition(thread,value); }
bool GameShapeAdapter::isTransitioning(const ShapeAnimationServices& s) const { return instance && thread && s.isTransitioning(thread); }
void GameShapeAdapter::startPlayback(const ShapeAnimationServices& s) {
    if (!thread) return;
    playbackFlags|=1; wasTransitioning=isTransitioning(s);
    float time=static_cast<float>(getTime(s));
    remainingPlaybackFrames=1-static_cast<int>((getDuration(s)-time)*-30.0L);
    if (remainingPlaybackFrames<1) remainingPlaybackFrames=1;
    if (remainingPlaybackFrames>450) remainingPlaybackFrames=450;
}
void GameShapeAdapter::advanceTime(float value,const ShapeAnimationServices& s) {
    if ((!thread || !isTransitioning(s)) && !(playbackFlags&1)) return;
    s.advanceAll(instance,value);
    if ((playbackFlags&1) && --remainingPlaybackFrames<1) {
        if (wasTransitioning) startPlayback(s); else playbackFlags&=~1u;
    }
}
}
