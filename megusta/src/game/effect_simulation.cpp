#include "effect.h"
#include <sg.h>
#include <cstdlib>
namespace megusta {
extern EffectRuntime* effectRuntime;
// 00440210 stores the lifetime pair with MOV dword writes, while the
// decompiler's float-pointer view results from its particle-record aliasing.
// The integer fields here match those instruction stores and the readers in
// the draw routines.
void Effect::advanceParticles() {
    if (style==8) setPosition(static_cast<float>(effectRuntime->mouseX),
        static_cast<float>(effectRuntime->screenHeight-effectRuntime->mouseY),0,0);
    if (emissionState==0) return;
    float emitted=0;
    int surviving=0;
    auto randomComponent=[](float range,float center) -> float {
        return static_cast<int>((std::rand()&255)-128)*range*0.039+center;
    };
    for (auto& particle:particles) {
        if (particle.remainingTicks>1) {
            bool update=false, screen=false;
            switch (style) {
            case 1:
                sgPreMultMat4(reinterpret_cast<float (*)[4]>(particle.transform),
                    reinterpret_cast<const float (*)[4]>(particle.rotationStepMatrix));
                update=true; break;
            case 2: case 4: case 7: case 9: update=true; break;
            case 3: case 8: update=true; screen=true; break;
            case 5:
                if (ground[2]==0 || ground[2]<particle.transform[14]) {
                    sgPreMultMat4(reinterpret_cast<float (*)[4]>(particle.transform),
                        reinterpret_cast<const float (*)[4]>(particle.rotationStepMatrix));
                    update=true;
                } else {
                    ++surviving; --particle.remainingTicks;
                }
                break;
            default: break;
            }
            if (update) {
                const int dimensions=screen ? 2 : 3;
                for (int axis=0;axis<dimensions;++axis)
                    particle.transform[12+axis]+=particle.velocity[axis];
                for (int axis=0;axis<dimensions;++axis)
                    particle.velocity[axis]=friction[axis]*particle.velocity[axis]+gravity[axis];
                float speedSquared=particle.velocity[0]*particle.velocity[0]+
                    particle.velocity[1]*particle.velocity[1];
                if (!screen) speedSquared=particle.velocity[2]*particle.velocity[2]+
                    particle.velocity[0]*particle.velocity[0]+particle.velocity[1]*particle.velocity[1];
                if (speedSquared<minimumSpeedSquaredThreshold) particle.remainingTicks=0;
                else { ++surviving; --particle.remainingTicks; }
            }
            // Live particles never enter the spawn branch on this iteration.
            continue;
        }
        if (emissionState<=0 || !(emitted<emissionRate)) continue;
        if (style<1 || style>9) continue;
        const bool rotating=style==1 || style==5;
        const bool local=style==9;
        const bool screen=style==3 || style==8;
        if (rotating || local) {
            for (int i=0;i<16;++i) particle.transform[i]=(i%5==0) ? 1.0f : 0.0f;
        }
        if (rotating) {
            int roll=std::rand(),pitch=std::rand(),heading=std::rand();
            sgMakeCoordMat4(reinterpret_cast<float (*)[4]>(particle.rotationStepMatrix),0,0,0,
                (heading&255)*0.00390625f,(pitch&255)*0.00390625f,(roll&255)*0.00390625f);
        }
        int ticks=durationTicks;
        if (style!=4 && style!=9) ticks+=std::rand()%32;
        particle.remainingTicks=ticks; particle.initialTicks=ticks;
        const int dimensions=screen ? 2 : 3;
        for (int axis=0;axis<dimensions;++axis)
            particle.transform[12+axis]=randomComponent(emissionVolume[axis],local ? 0 : position[axis]);
        if (style!=5 && style!=9)
            for (int axis=0;axis<dimensions;++axis) particle.spawnPosition[axis]=particle.transform[12+axis];
        for (int axis=0;axis<dimensions;++axis)
            particle.velocity[axis]=randomComponent(dispersion[axis],direction[axis]);
        emitted+=1;
        particle.colorRGB[0]=1; particle.colorRGB[1]=1; particle.colorRGB[2]=1;
        particle.spawnAlpha=1;
    }
    if (emissionState==-1 && surviving==0) emissionState=0;
}
}
