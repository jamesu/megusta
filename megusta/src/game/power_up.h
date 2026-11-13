// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "spatial_object.h"
#include <cstdint>
class ssgEntity;
class ssgTransform;
class ssgAnim;
class fntRenderer;
namespace megusta {
class Random;
class PowerUps;
class Task;
struct PowerUpsRuntime;
// The same native record is used for spatial powerups, coins and player slots.
class PowerUp : public SpatialObject {
    friend class PowerUps;
public:
    PowerUp() = default;
    explicit PowerUp(Random&);
    const char* typeTag() const override;
    void cycleDefinition();
    void updateModelTransform();
    void translate(float,float,float) override;
    int writeRecord(char*,std::uint32_t) const override;
    int parseRecord(const char*,std::uint32_t) override;
    void drawDebug(const float* cameraMatrix);
    void drawEditorInfo(PowerUpsRuntime&,fntRenderer&);
    int definitionId=0;
    char tagPrefixStorage[12];
    float position[3]{},groundPosition[3]{};
    unsigned randomVariant=0,randomDirection=0,rotationTick=0;
    float rotation=0;
    Task* pickupTask=nullptr;
    ssgEntity *model=nullptr,*glowModel=nullptr;
    ssgTransform *primary=nullptr,*secondary=nullptr,*tertiary=nullptr;
    ssgAnim *animator=nullptr,*secondaryAnimator=nullptr,*tertiaryAnimator=nullptr;
    bool active=false;
};
}
