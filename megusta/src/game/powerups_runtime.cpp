#include "powerups.h"
#include "random.h"
#include "balls.h"
#include "ball_collision.h"
#include "scene_runtime.h"
#include "room_client.h"
#include "task.h"
#include "native_game_helpers.h"
#include "recovered_binding_primitives.h"
#include "engine/opengl_adapter.h"
#include "engine/resource_runtime.h"
#include "engine/runtime_adapters.h"
#include "engine/string_services.h"
#include "platform/game_bridge.h"
#include "gui_assets.h"
#include "ssgGameAnimation.h"
#include "fnt.h"
#include "../engine/platform_gl.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace megusta {
namespace {
PowerUpsRuntime* runtime=nullptr;
void showPowerMessage(const char* text) {
    auto& r=powerUpsRuntime(); r.players.showMessage(text,20,r.fixedTick);
}
void makePowerTransform(ssgTransform* transform,const float* position,float angle) {
    sgMat4 rotation,translation; sgVec3 axis={0,0,1};
    sgMakeRotMat4(rotation,angle,axis); sgMakeTransMat4(translation,position[0],position[1],position[2]);
    sgPostMultMat4(rotation,translation); transform->setTransform(rotation);
}
bool castPowerGround(const Point3F& start,const Point3F& end,BallContactResult& result) {
    auto& r=powerUpsRuntime(); return r.collision.castSweptBall(r.objects,r.grid,start,end,result);
}
void stepPowerAnimation(ssgAnim* animation) {
    unsigned frame=animation->getFrame()+1;
    if (frame>=static_cast<unsigned>(animation->getNumFrames())) frame=0;
    animation->setFrame(frame);
}
}
void bindPowerUpsRuntime(PowerUpsRuntime& state) { runtime=&state; }
PowerUpsRuntime& powerUpsRuntime() {
    if (!runtime) throw std::logic_error("Powerups require the shared gameplay, scene, IO and network runtime");
    return *runtime;
}
PowerUp::PowerUp(Random& random) : PowerUp() {
    radius_=0; randomVariant=random.generate()%5; randomDirection=random.generate()&1;
    rotation=static_cast<float>(random.generate()%360);
}
const char* PowerUp::typeTag() const { return tagPrefixStorage; }
void PowerUp::cycleDefinition() {
    if (definitionId < powerUpsRuntime().powers.powerDefinitionCount - 1) ++definitionId;
    else definitionId=0;
}
void PowerUp::updateModelTransform() {
    if (!primary) return;
    makePowerTransform(primary,position,rotation);
    if (std::strcmp(typeTag(),"PowerUp")) return;
    if (secondary) makePowerTransform(secondary,position,rotation);
    // The original instructions store position.x/y/z contiguously at the
    // sweep start; the endpoint copies X/Y and subtracts 50 from Z.
    BallContactResult hit;
    if (castPowerGround({position[0],position[1],position[2]},
                        {position[0],position[1],static_cast<float>(position[2]-50)},hit)) {
        std::memcpy(groundPosition,hit.position,sizeof(groundPosition));
        makePowerTransform(tertiary,groundPosition,rotation);
        if (powerUpsRuntime().powers.powersEnabled) tertiary->setTraversalMaskBits(1);
    } else tertiary->clrTraversalMaskBits(1);
}
void PowerUp::translate(float x,float y,float z) {
    transform_[3][0]+=x; transform_[3][1]+=y; transform_[3][2]+=z;
    std::memcpy(position,transform_[3],sizeof(position)); updateModelTransform();
}
int PowerUp::writeRecord(char* text,std::uint32_t) const {
    // Decoded from the saved Ghidra string label at 004f3f28. Its separator
    // pattern matches the known neighboring literal at 004f3f44.
    static constexpr char format[]=" %d, %f, %f, %f, %.01f \n";
    std::strcpy(text,typeTag());
    std::sprintf(text+std::strlen(typeTag()),format,spatialId_,
        static_cast<double>(position[0]),static_cast<double>(position[1]),static_cast<double>(position[2]),static_cast<double>(radius_));
    return static_cast<int>(std::strlen(text));
}
int PowerUp::parseRecord(const char* text,std::uint32_t) {
    auto& powers=powerUpsRuntime().powers;
    if (powers.powerInstanceCount>=10) return 0;
    powers.attachPowerUpRecord(this);
    std::sscanf(text+std::strlen(typeTag()),"%d,%f,%f,%f,%f",&spatialId_,position,position+1,position+2,&radius_);
    sgMakeIdentMat4(transform_); std::memcpy(transform_[3],position,sizeof(position));
    updateModelTransform(); return 1;
}
void PowerUps::releaseHole() {
    for (int i=0;i<powerDefinitionCount;++i) delete powerDefinitions[i];
    ssgDeRefDelete(powerBallModel); powerBallModel=nullptr; ssgDeRefDelete(glowModel); glowModel=nullptr;
    for (int i=0;i<coinDefinitionCount;++i) delete coinDefinitions[i];
    for (int i=0;i<coinInstanceCount;++i) delete coinInstances[i];
    for (auto*& model:coinModels) { ssgDeRefDelete(model); model=nullptr; }
    glDeleteTextures(1,&groundMarkerTexture);
    powerDefinitionCount=powerInstanceCount=coinInstanceCount=coinDefinitionCount=lastUsedPowerUpId=0;
}
void PowerUps::loadHoleModels() {
    powerBallModel=loadSceneModel("powerups/PWRBALL",true); powerBallModel->ref();
    glowModel=loadSceneModel("powerups/iflGlow",true); glowModel->ref();
    const char* names[3]={"coin03","coin02","coin01"};
    for (int i=0;i<3;++i) { coinModels[i]=loadSceneModel(names[i],true); coinModels[i]->ref(); }
    // Runtime mounted readers are shared with the scene loaders; no temp file.
    // The recovered call is glpng_pngBind(path, 0, PNG_ALPHA, ...). The
    // adapter supplies PNG_ALPHA internally, so its mipmap/options argument
    // must remain zero here.
    groundMarkerTexture=pix::bindPngTexture(powerUpsRuntime().resources,"DATA/TEXTURES/GOLFBALS.PNG",0,GL_CLAMP,GL_NEAREST,GL_NEAREST);
}
void PowerUps::attachPowerUpRecord(PowerUp* power) {
    if (powerUpsRuntime().powers.powerInstanceCount>=10) { showPowerMessage(">>Max Hole Powerups Reached<<"); return; }
    for (int i=0;i<powerInstanceCount;++i) {
        const auto* existing=powerInstances[i];
        if ((std::fabs(existing->transform_[3][0]-power->position[0])<1e-6 &&
             std::fabs(existing->transform_[3][1]-power->position[1])<1e-6 &&
             std::fabs(existing->transform_[3][2]-power->position[2])<1e-6) || existing==power) return;
    }
    power->definitionId=0; std::strcpy(power->tagPrefixStorage,"PowerUp");
    power->active=true; power->radius_=5; power->pickupTask=defaultPickupTask;
    power->model=powerBallModel;
    power->primary=createSceneTransform(power->position[0],power->position[1],power->position[2],0);
    power->primary->addKid(power->model->getByName("ro_PWRBALL_base")); gameSceneRoots.root->addKid(power->primary);
    power->animator=createSceneAnimation(power->primary);
    power->glowModel=glowModel;
    power->secondary=createSceneTransform(power->position[0],power->position[1],power->position[2],0);
    power->secondary->addKid(power->glowModel->getByName("glow_aim")); gameSceneRoots.root->addKid(power->secondary);
    power->secondaryAnimator=createSceneAnimation(power->secondary);
    if (powerInstanceCount<10) powerInstances[powerInstanceCount++]=power;
    power->tertiary=createSceneTransform(power->groundPosition[0],power->groundPosition[1],power->groundPosition[2],0);
    power->tertiary->addKid(power->model->getByName("ro_PWRBALL_ifl_ShadowPlane")); gameSceneRoots.root->addKid(power->tertiary);
    power->tertiaryAnimator=createSceneAnimation(power->tertiary); power->updateModelTransform();
    if (!powersEnabled) {
        power->active=false; power->primary->clrTraversalMaskBits(1);
        power->secondary->clrTraversalMaskBits(1); power->tertiary->clrTraversalMaskBits(1);
    }
}
void PowerUps::addHoleCoin(int id,float x,float y,float z,float radius) {
    if (!coinsEnabled) return;
    if (coinInstanceCount>299) { showPowerMessage(">>Max Hole Coins Reached<<"); return; }
    auto* coin=new PowerUp(powerUpsRuntime().random); coin->definitionId=id; std::strcpy(coin->tagPrefixStorage,"Coin");
    coin->active=true; coin->position[0]=x; coin->position[1]=y; coin->position[2]=z;
    coin->radius_=radius; coin->pickupTask=coinPickupTask; coin->model=coinModels[id];
    coin->primary=createSceneTransform(x,y,z,0); coin->primary->addKid(coin->model);
    coin->animator=createSceneAnimation(coin->primary); gameSceneRoots.root->addKid(coin->primary);
    if (coinInstanceCount<300) coinInstances[coinInstanceCount++]=coin;
    BallContactResult hit;
    if (castPowerGround({x,y,z},{x,y,-50},hit)) std::memcpy(coin->groundPosition,hit.position,sizeof(coin->groundPosition));
    coin->updateModelTransform();
}
void PowerUps::processPickups() {
    if ((!powersEnabled && !coinsEnabled) || isEditing()) return;
    auto& r=powerUpsRuntime(); auto* player=r.players.currentPlayer;
    if (player->powerUpActivationPending && powersEnabled && !(player->getBall(r.balls)->flags&0x10)) {
        const unsigned id=player->powerUps[player->selectedPowerUpSlot].definitionId;
        bool activate=r.players.turnState==4 || r.powers.getDefinitionKind(id)==0;
        if (!activate) for (int i=0;i<r.powers.powerDefinitionCount;++i) {
            if (id==r.powers.powerDefinitions[i]->id) { activate=r.powers.powerDefinitions[i]->kind==2; break; }
        }
        if (activate) for (int i=0;i<r.powers.powerDefinitionCount;++i) {
            auto* definition=r.powers.powerDefinitions[i]; if (definition->id==id && definition->task) definition->task->start(r.tasks);
        }
    }
    auto* ball=player->getBall(r.balls);
    if (!ball || (ball->flags&10) || !r.network->isVolatileSafe()) return;
    const auto visit=[&](PowerUp** instances,int& count) {
        for (currentInstanceIndex=0;currentInstanceIndex<count;++currentInstanceIndex) {
            auto* power=instances[currentInstanceIndex];
            if (power->active && (PowerUps_collide_bounds_impl(power->position[0],power->position[1],power->position[2],
                ball->position[0],ball->position[1],ball->position[2],power->radius_)&0xff) && power->pickupTask)
                power->pickupTask->start(r.tasks);
        }
    };
    if (powersEnabled) visit(powerInstances,powerInstanceCount);
    if (coinsEnabled) visit(coinInstances,coinInstanceCount);
}
void PowerUps::handleInput() {
    if ((!powersEnabled && !coinsEnabled) || isEditing()) return;
    auto& r=powerUpsRuntime(); const auto key=r.input.keyCode;
    if (isEditing() || r.network->shouldWaitForRemoteTurn() || r.network->hasPendingStartState() || !r.players.currentPlayer->isHuman) return;
    auto* player=r.players.currentPlayer; const unsigned edges=r.input.buttons&~r.input.previousButtons;
    if ((edges&1) || (key==9 && (r.input.modifiers&4))) {
        player->selectPreviousPowerUp(r.playerServices); r.network->sendPreviousPowerUp();
    } else if ((edges&2) || key==9) { player->usedPowerUp(r.playerServices); r.network->sendNextPowerUp(); }
    if (((edges&0x20) || key==0x20) && !player->powerUpActivationPending && !lastUsedPowerUpId && player->powerUps[player->selectedPowerUpSlot].definitionId) {
        player->powerUpActivationPending=true; r.network->sendActivatePowerUp();
    }
}
void PowerUps::advanceAnimations() {
    if (powersEnabled) for (int i=0;i<powerInstanceCount;++i) if (powerInstances[i]->active) {
        auto* power=powerInstances[i]; stepPowerAnimation(power->animator);
        stepPowerAnimation(power->secondaryAnimator); stepPowerAnimation(power->tertiaryAnimator);
    }
    if (coinsEnabled) for (int i=0;i<coinInstanceCount;++i) {
        auto* coin=coinInstances[i];
        if (!coin->active && coin->primary->getTraversalMask()!=1) {
            stepPowerAnimation(coin->animator); if (!coin->animator->getFrame()) hideCoinTransform(i);
        }
        if (coin->active && ++coin->rotationTick>3) {
            coin->rotationTick=0; coin->rotation+=coin->randomDirection?-10.0f:10.0f;
            if (coin->rotation<0) coin->rotation=360; else if (coin->rotation>360) coin->rotation=0;
            coin->updateModelTransform();
        }
    }
    drawCoinGroundMarkers(); if (generatingCoinTable) generateCoinTableStep();
}
void PowerUps::drawCoinGroundMarkers() {
    if (!coinInstanceCount) return;
    glEnable(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glBindTexture(GL_TEXTURE_2D,groundMarkerTexture);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glLoadMatrixf(powerUpsRuntime().cameraMatrix);
    glBegin(GL_QUADS); glColor4f(0,0,0,1);
    for (int i=0;i<coinInstanceCount;++i) if (coinInstances[i]->active) {
        const float* p=coinInstances[i]->groundPosition;
        glTexCoord2f(0,0); glVertex3f(p[0]-1,p[1]-1,p[2]); glTexCoord2f(1,0); glVertex3f(p[0]+1,p[1]-1,p[2]);
        glTexCoord2f(1,1); glVertex3f(p[0]+1,p[1]+1,p[2]); glTexCoord2f(0,1); glVertex3f(p[0]-1,p[1]+1,p[2]);
    }
    glEnd();
}
void PowerUp::drawDebug(const float* cameraMatrix) {
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glLoadMatrixf(cameraMatrix); glMultMatrixf(&transform_[0][0]);
    glDisable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    if (powerUpsRuntime().spatialObjects.validateSelection()==this) glColor4f(0.8f,0.8f,static_cast<float>(std::rand()&255)*0.00390625f,1);
    else glColor4f(0,1,0,1);
    powerUpsRuntime().gl.drawSphere(radius_,12,12); glEnable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}
void PowerUp::drawEditorInfo(PowerUpsRuntime& r,fntRenderer& renderer) {
    const auto* ball=r.players.currentPlayer->getBall(r.balls);
    const float x=ball->position[0]-position[0],y=ball->position[1]-position[1],z=ball->position[2]-position[2];
    const float distance=static_cast<float>(std::sqrt(x*x+y*y+z*z));
    char text[84]; std::sprintf(text,"Edit - %s:%d Type:%s Distance:%f",typeTag(),spatialId_,r.powers.getName(definitionId),static_cast<double>(distance));
    renderer.setPointSize(14); renderer.start2f(16,90); renderer.begin(); renderer.puts(text);
    std::sprintf(text,"X:%f Y:%f Z:%f Radius:%.01f",static_cast<double>(position[0]),static_cast<double>(position[1]),
        static_cast<double>(position[2]),static_cast<double>(radius_));
    renderer.start2f(16,74); renderer.puts(text); renderer.end();
}
void PowerUps::drawEditorBallPosition(fntRenderer& renderer,fntFont* font) {
    if (!isEditing()) return;
    renderer.setFont(font); renderer.setPointSize(24); glColor3f(0,0,0); renderer.setPointSize(10);
    auto& r=powerUpsRuntime(); const auto* ball=r.players.currentPlayer->getBall(r.balls);
    char text[80]; std::sprintf(text,"Ball is at %fX %fY %fZ",static_cast<double>(ball->position[0]),
        static_cast<double>(ball->position[1]),static_cast<double>(ball->position[2]));
    renderer.start2f(250,20); renderer.begin(); renderer.puts(text); renderer.end();
}
void PowerUps::generateCoinTableStep() {
    auto& r=powerUpsRuntime(); char path[256];
    if (-generationRadius==generationOffsetX) {
        std::sprintf(path,"data/coins/coin_%s.dat",r.mapName); coinTableFile=sharedIo->open(path,"wb");
    }
    if (!coinTableFile) return;
    std::sprintf(path,"tee_0%i",generationTeeIndex); const float* tee=r.objects.findByName(path)->getPosition();
    const Point3F origin{tee[0],tee[1],tee[2]};
    if (generationOffsetY>=generationRadius) {
        generationOffsetY=-generationRadius;
        if (generationOffsetX>=generationRadius) { generationOffsetX=-generationRadius; generatingCoinTable=false; }
        else { generationOffsetX+=3; if (!r.generationPreviousHit) generationOffsetX+=3; }
    } else { generationOffsetY+=3; if (!r.generationPreviousHit) generationOffsetY+=3; }
    r.generationPreviousHit=0;
    if (!generatingCoinTable) {
        sharedIo->stdioPrintf("COINAGE: Table Generated - %d Coin Locations\n",generatedCoinCount);
        sharedIo->close(coinTableFile); return;
    }
    const Point3F start{static_cast<float>(origin.x+generationOffsetX),static_cast<float>(origin.y+generationOffsetY),static_cast<float>(origin.z+120)};
    const Point3F end{start.x,start.y,static_cast<float>(origin.z-120)}; BallContactResult hit;
    float z=origin.z;
    if (castPowerGround(start,end,hit)) {
        glColor4f(0,1,0,1); char name[256]; std::strcpy(name,r.objects.objects[hit.objectIndex].name);
        strings::uppercase(name);
        if (std::strstr(name,generationSurfaceName)) {
            glColor4f(1,0,0,1); r.generationPreviousHit=1;
            const Point3F position{start.x,start.y,hit.position[2]}; sharedIo->write(&position,12,1,coinTableFile); ++generatedCoinCount;
        }
        z=hit.position[2];
    } else glColor4f(0,0,1,1);
    sgMat4 matrix; sgMakeTransMat4(matrix,start.x,start.y,z);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glLoadMatrixf(r.cameraMatrix); glMultMatrixf(&matrix[0][0]);
    glDisable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); r.gl.drawSphere(2,12,12);
    glEnable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}
void PowerUps::deployCoins(int first,int second,int third,int mode) {
    if (!coinsEnabled || (!first && !second && !third)) return;
    auto& r=powerUpsRuntime(); const auto initialCount=coinInstanceCount;
    sharedIo->stdioPrintf("COINAGE: rnd.seed = %d\n",static_cast<int>(r.random.state));
    const float* tee=r.objects.findByName("tee_01")->getPosition(); const Point3F origin{tee[0],tee[1],tee[2]};
    char path[256];
    if (!mode) {
        std::strcpy(path,"data/coins/coin_"); std::strncat(path,r.mapName,255); path[255]='\0';
        std::strncat(path,".dat",255); path[255]='\0'; coinTableFile=sharedIo->open(path,"rb");
        if (!coinTableFile) { sharedIo->stdioPrintf("COINS: %s not found\n",path); mode=1; }
    }
    const std::uint32_t placementStart=platform::gameMilliseconds();
    if (mode==23) {
        int placed=0;
        for (int attempt=0;attempt<=398 && placed<10 && r.powers.powerInstanceCount<10;++attempt) {
            const float y=static_cast<float>((std::rand()&511)-256),x=static_cast<float>((std::rand()&511)-256);
            BallContactResult hit;
            if (!castPowerGround({x,y,-1000},{x,y,1000},hit) ||
                strings::compareInsensitiveBounded(r.objects.objects[hit.objectIndex].name,"green",5)) continue;
            auto* power=static_cast<PowerUp*>(SpatialObjects::createFromTag("PowerUp"));
            r.spatialObjects.insert(power); sgMakeIdentMat4(power->transform_);
            power->transform_[3][0]=hit.position[0]; power->transform_[3][1]=hit.position[1]; power->transform_[3][2]=hit.position[2]+3.5f;
            r.powers.attachPowerUpRecord(power); ++placed;
        }
        return;
    }
    Point3F positions[5000];
    if (mode!=1) {
        generatedCoinCount=sharedIo->read(positions,12,5000,coinTableFile); sharedIo->close(coinTableFile);
        sharedIo->stdioPrintf("COINAGE: read %d coins positions\n",generatedCoinCount);
    }
    const int counts[3]={first,second,third};
    for (int kind=2;kind>=0;--kind) for (int i=0;i<counts[kind] && coinInstanceCount<300;++i)
        addHoleCoin(kind,0,0,origin.z+120,6);
    int remaining=first+second+third,index=initialCount;
    if (mode==1) {
        while (remaining && index<=299) {
            auto* coin=coinInstances[index];
            coin->position[0]=static_cast<float>(static_cast<long double>(origin.x)+800-2*(r.random.generate()%800));
            coin->position[1]=static_cast<float>(static_cast<long double>(origin.y)+800-2*(r.random.generate()%800));
            BallContactResult hit;
            if (!castPowerGround({coin->position[0],coin->position[1],coin->position[2]},
                                 {coin->position[0],coin->position[1],static_cast<float>(origin.z-120)},hit)) continue;
            char name[256]; std::strcpy(name,r.objects.objects[hit.objectIndex].name); strings::uppercase(name);
            if (!std::strstr(name,"GREEN")) continue;
            bool close=false;
            for (int i=0;i<index;++i) {
                const float dx=coinInstances[i]->position[0]-coin->position[0],dy=coinInstances[i]->position[1]-coin->position[1];
                if (std::sqrt(dx*dx+dy*dy)<3) { close=true; break; }
            }
            if (close) continue;
            std::memcpy(coin->groundPosition,coin->position,sizeof(coin->position));
            coin->position[2]=hit.position[2]+1.5f; coin->groundPosition[2]=hit.position[2]; ++index; --remaining;
        }
    } else if (remaining<generatedCoinCount && remaining) {
        for (unsigned attempt=0;remaining && index<=299;++attempt) {
            if (attempt>2000) { sharedIo->stdioPrintf("COINAGE: Placement #0 - Timed out\n"); break; }
            const unsigned candidate=generatedCoinCount<2?0:r.random.generate()%generatedCoinCount;
            auto& point=positions[candidate]; if (!point.x) continue;
            auto* coin=coinInstances[index]; coin->position[0]=point.x; coin->position[1]=point.y; coin->position[2]=point.z+1.5f;
            std::memcpy(coin->groundPosition,coin->position,sizeof(coin->position)); coin->groundPosition[2]=point.z;
            sharedIo->stdioPrintf("COINAGE: val = %d makes hole_coins[%.4d] = (%.4f,%.4f,%.4f)\n",
                static_cast<int>(candidate),index,static_cast<double>(coin->position[0]),
                static_cast<double>(coin->position[1]),static_cast<double>(coin->position[2]));
            point.x=0; ++index; --remaining;
        }
    }
    const int elapsed=static_cast<int>(platform::gameMilliseconds()-placementStart);
    const char* summary=engine_runtime::engineString_formatStatic_impl(
        mode==1 ? "Coin Placement #1 - %i\n" : "COINAGE: Placement #0 - %i\n",elapsed);
    sharedIo->stdioPrintf("%s",summary);
}
}
