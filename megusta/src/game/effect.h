#pragma once
#include <cstdint>
namespace megusta {
class GameConfig;
class OpenGLAdapter;
class Effects;
struct EffectLensEntry { float intensity,positionFactor,sizeFactor; };
extern EffectLensEntry effectLensRecords[7];
struct EffectRuntime {
    Effects& effects;
    GameConfig& config;
    OpenGLAdapter& gl;
    int& mouseX;
    int& mouseY;
    int& screenHeight;
    const float* cameraMatrix;
    std::uint32_t sharedTexture1, sharedTexture0, sharedTexture3, lensTexture;
};
// Explicit host wiring for recovered shared globals; bind before use.
void bindEffectRuntime(EffectRuntime&);
struct EffectParticle {
    float transform[16], rotationStepMatrix[16];
    float spawnPosition[3], velocity[3], colorRGB[3];
    float unresolved_a4, spawnAlpha;
    std::int32_t remainingTicks, initialTicks;
};
class Effect {
public:
    EffectParticle particles[256];
    float position[3], emissionVolume[3], direction[3], angleDegrees;
    float gravity[3], scale[3], dispersion[3], ground[3], friction[3];
    std::uint8_t unresolved_b464[20];
    float emissionRate, minimumSpeedSquaredThreshold;
    std::uint8_t unresolved_b480[8];
    std::int32_t rampEnabled, emissionState, durationTicks, style, blendMode;
    std::uint32_t textureId;
    Effect();
    explicit Effect(float unregisteredStyle);
    void initializeUnregistered(float);
    void releaseTexture();
    void setTexture(const char*);
    void setRate(float);
    void warmup(int);
    void advanceParticles();
    void drawOverlayStyles();
    void drawWorldStyles();
    void drawScreenParticles();
    void drawOrientedParticles();
    void drawLocalParticles();
    void start();
    void stop();
    void setPosition(float,float,float,float);
    void setVolume(float,float,float);
    void setGround(float,float,float);
    void setDirection(float,float,float);
    void setDispersion(float,float,float);
    void setGravity(float,float,float);
    void setRamp(float);
    void setScale(float);
    void setDuration(float);
    void setMinVel(float);
    void setStyle(float);
    void setBlend(float);
    void setFriction(float);
private:
    void initializeFields();
};
class Effects {
public:
    std::int32_t effectCount;
    Effect* effects[64];
    std::uint8_t unresolved_104[8];
    std::int32_t detailLevel;
    int getDetailLevel();
    void initializeLensTexture();
    void insert(Effect*);
    void release();
    void updateAll();
    void drawAll();
    void drawLensPassAll();
};
static_assert(sizeof(EffectParticle)==180,"Recovered particle stride");
static_assert(sizeof(Effect)==46240,"Recovered emitter extent");
}
