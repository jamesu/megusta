#include "powerups.h"
#include "ssg.h"
#include "engine/runtime_adapters.h"
#include <cstring>
namespace megusta {
void PowerUps::initialize() {
    powerDefinitionCount=0; powerInstanceCount=0;
    coinDefinitionCount=0; coinInstanceCount=0;
    powersEnabled=true; coinsEnabled=true; slowMotion=false;
    slowMotionTickDivider=0; lastUsedPowerUpId=0;
    generatingCoinTable=false;
    generationRadius=0.0f; generationOffsetX=0.0f; generationOffsetY=0.0f;
}
unsigned PowerUps::getDefinitionKind(int id) const {
    for (int i=0;i<powerDefinitionCount;++i) {
        if (powerDefinitions[i]->id==id) return powerDefinitions[i]->kind;
    }
    return 0xffffffffu;
}
void PowerUps::hideCoinTransform(int index) {
    coinInstances[index]->primary->clrTraversalMaskBits(1);
}
int PowerUps::createPower(unsigned kind,const char* name,Task* task) {
    if (powerDefinitionCount>=10) return -1;
    auto* definition=new PowerUpDefinition;
    std::strcpy(definition->name,name); definition->id=powerDefinitionCount; definition->kind=kind; definition->task=task;
    powerDefinitions[powerDefinitionCount++]=definition; return definition->id;
}
void PowerUps::createCoin(int id,unsigned kind,const char* name) {
    auto* definition=new PowerUpDefinition;
    std::strcpy(definition->name,name); definition->id=id; definition->kind=kind; definition->task=nullptr;
    if (coinDefinitionCount<3) coinDefinitions[coinDefinitionCount++]=definition;
}
const char* PowerUps::getName(int id) const {
    if (id<0 || id>powerDefinitionCount) engine_runtime::engineRuntime_fatalExit_impl();
    for (int i=0;i<powerDefinitionCount;++i) if (powerDefinitions[i]->id==id) return powerDefinitions[i]->name;
    return nullptr;
}
const char* PowerUps::getCoinName(int id) const {
    if (id<0 || id>coinDefinitionCount) engine_runtime::engineRuntime_fatalExit_impl();
    for (int i=0;i<coinDefinitionCount;++i) if (coinDefinitions[i]->id==id) return coinDefinitions[i]->name;
    return nullptr;
}
int PowerUps::getCurrentPowerUp() const { return powerInstances[currentInstanceIndex]->definitionId; }
int PowerUps::getCurrentCoin() const { return coinInstances[currentInstanceIndex]->definitionId; }
void PowerUps::generateCoinTable(float radius,int teeIndex,const char* surface) {
    if(radius<=0.0f) {
        generationRadius=0.0f; generationTeeIndex=1; generationOffsetX=0.0f; generationOffsetY=0.0f;
        std::strcpy(generationSurfaceName,"GREEN"); generatingCoinTable=false; return;
    }
    generationRadius=radius; generationOffsetX=-radius; generationOffsetY=-radius; generationTeeIndex=teeIndex;
    std::strcpy(generationSurfaceName,surface?surface:"GREEN"); generatingCoinTable=true; generatedCoinCount=0;
}
void PowerUps::removeCurrentPowerUp() {
    auto& instance=*powerInstances[currentInstanceIndex]; instance.active=false;
    instance.primary->clrTraversalMaskBits(1); instance.secondary->clrTraversalMaskBits(1); instance.tertiary->clrTraversalMaskBits(1);
}
void PowerUps::removeCurrentCoin() { coinInstances[currentInstanceIndex]->active=false; }
void PowerUps::reset() {
    if (!powersEnabled) return;
    for (int i=0;i<powerInstanceCount;++i) {
        auto& instance=*powerInstances[i]; instance.active=true;
        instance.primary->setTraversalMaskBits(1); instance.secondary->setTraversalMaskBits(1); instance.tertiary->setTraversalMaskBits(1);
    }
}
}
