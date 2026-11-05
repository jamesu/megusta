#include "effect.h"
#include "engine/multi_texture.h"
#include "game_config.h"
#include "../engine/opengl_adapter.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glpng.h>
#include <cstdio>
#include <cstring>
namespace megusta {
// Initial contents exported from the canonical Ghidra program at 0050394c.
// drawWorldStyles_impl changes each intensity field to 1 while rendering.
EffectLensEntry effectLensRecords[7]={
    {1.0f, 1.0f, 1.0f},
    {2.0f, 0.05f, 0.5f},
    {3.0f, 0.3f, 0.25f},
    {4.0f, 0.1f, 0.5f},
    {3.0f,-0.5f, 0.25f},
    {2.0f,-0.2f, 0.25f},
    {1.0f,-0.1f, 0.25f},
};
EffectRuntime* effectRuntime=nullptr;
void bindEffectRuntime(EffectRuntime& runtime) { effectRuntime=&runtime; }
void Effect::initializeFields() {
    setVolume(0,0,0); setDirection(0,0,0); angleDegrees=0;
    setDispersion(0,0,0); setGravity(0,0,-0.01f); setScale(0);
    setGround(0,0,0); setFriction(0);
    std::memset(unresolved_b464,0,sizeof unresolved_b464);
    std::memset(unresolved_b480,0,sizeof unresolved_b480);
    rampEnabled=0; emissionState=0; textureId=0;
    durationTicks=50; blendMode=0; minimumSpeedSquaredThreshold=0.01f;
    // position, emissionRate, and inactive particle contents are untouched.
    for (auto& particle:particles) particle.remainingTicks=0;
}
Effect::Effect() { initializeFields(); style=2; effectRuntime->effects.insert(this); }
Effect::Effect(float value) { initializeUnregistered(value); }
void Effect::initializeUnregistered(float value) { initializeFields(); setStyle(value); }
int Effects::getDetailLevel() {
    if (detailLevel<0) detailLevel=effectRuntime->config.readInt("Prefs","DetailLevel",10);
    return detailLevel;
}
void Effects::initializeLensTexture() {
    detailLevel=-1;
    if (!effectRuntime->lensTexture) {
        pngInfo info;
        auto& gl=effectRuntime->gl;
        effectRuntime->lensTexture=pix::bindPngTexture("data/textures/effects/lens4.png",0,
            gl.tokens[0x58],gl.tokens[0x4f],gl.tokens[0x4f]);
    }
}
void Effects::insert(Effect* effect) {
    if (effectCount<64) effects[effectCount++]=effect;
    else std::printf("Effects::insert Number of Effects exceeds %d\n",64);
}
void Effects::release() {
    for (int i=0;i<effectCount;++i) {
        effects[i]->releaseTexture();
        // Original calls operator_delete without an additional destructor body.
        ::operator delete(effects[i]);
    }
    effectCount=0; detailLevel=-1;
    auto remove=[](std::uint32_t& texture) {
        if (texture) { effectRuntime->gl.deleteTextures(1,&texture); texture=0; }
    };
    remove(effectRuntime->sharedTexture1); remove(effectRuntime->sharedTexture0);
    remove(effectRuntime->sharedTexture3); remove(effectRuntime->lensTexture);
}
void Effects::updateAll() { for (int i=0;i<effectCount;++i) effects[i]->advanceParticles(); }
void Effects::drawAll() { for (int i=0;i<effectCount;++i) effects[i]->drawOverlayStyles(); }
void Effects::drawLensPassAll() { for (int i=0;i<effectCount;++i) effects[i]->drawWorldStyles(); }
void Effect::releaseTexture() {
    if (textureId) { effectRuntime->gl.deleteTextures(1,&textureId); textureId=0; }
}
void Effect::setTexture(const char* path) {
    pngInfo info;
    textureId=pix::bindPngTexture(path,0,effectRuntime->gl.tokens[0x58],
        effectRuntime->gl.tokens[0x4f],effectRuntime->gl.tokens[0x4f]);
}
void Effect::setRate(float value) {
    emissionRate=value;
    if (value>0) {
        int detail=effectRuntime->effects.getDetailLevel();
        if (detail<1) { emissionRate=-1; return; }
        emissionRate=static_cast<float>(detail)*value*0.1;
    }
}
void Effect::warmup(int count) { for (int i=0;i<count;++i) advanceParticles(); }
}
