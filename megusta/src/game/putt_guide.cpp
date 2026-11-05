#include "putt_guide.h"
#include <ssg.h>
#include "ball_collision.h"
#include "engine/engine_math.h"
#include "recovered_binding_primitives.h"
#include "vector_primitives.h"
#include <cmath>
namespace megusta {
void PuttGuide::updateTrajectory(const Point3F& origin,const Point3F& travel,SceneObjectCollection& objects,
                                 SceneGridIndex& grid,BallCollisionWorkspace& collision) {
    float remaining=binding_math_sqrtf(travel.x*travel.x+travel.z*travel.z+travel.y*travel.y);
    if (remaining<=0.01f) return;
    Point3F direction=travel,current=origin,directions[10]{};
    directions[0]=direction;
    float* first=linePositions->get(0); first[0]=origin.x; first[1]=origin.y; first[2]=origin.z;
    int index=1;
    for (;index<pointCount;++index) {
        Point3F end{static_cast<float>(current.x+direction.x),static_cast<float>(current.y+direction.y),static_cast<float>(current.z+direction.z)};
        BallContactResult contact{};
        bool hit=collision.castSweptBall(objects,grid,current,end,contact);
        if (hit) {
            collision.backOffBallContact(objects,grid,current,end,contact);
            float dot=contact.normal[0]*direction.x+contact.normal[1]*direction.y+contact.normal[2]*direction.z;
            float restitution=-objects.objects[contact.objectIndex].coefficientOfRestitution;
            end={contact.position[0],contact.position[1],contact.position[2]};
            direction.x=restitution*contact.normal[0]*dot+(direction.x-contact.normal[0]*dot);
            direction.y=contact.normal[1]*dot*restitution+(direction.y-contact.normal[1]*dot);
            direction.z=contact.normal[2]*dot*restitution+(direction.z-contact.normal[2]*dot);
        }
        directions[index]=direction;
        float* p=linePositions->get(index); p[0]=end.x; p[1]=end.y; p[2]=end.z;
        Point3F segment{static_cast<float>(current.x-end.x),
                        static_cast<float>(current.y-end.y),
                        static_cast<float>(current.z-end.z)};
        remaining-=game_vectors::Point3_length_impl(&segment);
        float squared=direction.x*direction.x+direction.z*direction.z+direction.y*direction.y;
        float length=binding_math_sqrtf(squared);
        current=end;
        if (remaining<=0.5f || length<=0.5f) { ++index; break; }
        // The saved body evaluates sqrtf again after the short-vector branch.
        length=binding_math_sqrtf(squared);
        if (length!=0) {
            const long double factor=static_cast<long double>(remaining)/length;
            direction.x=static_cast<float>(factor*direction.x); direction.y=static_cast<float>(factor*direction.y); direction.z=static_cast<float>(factor*direction.z);
        }
        if (!hit) { ++index; break; }
    }
    for (;index<pointCount;++index) {
        directions[index]=direction; float* p=linePositions->get(index); p[0]=current.x; p[1]=current.y; p[2]=current.z;
    }
    for (int i=0;i+1<pointCount;++i) {
        float angle=static_cast<float>(engine_math::engineMath_atan2_impl(directions[i].y,directions[i].x));
        long double x=engine_math::engineMath_sin_impl(angle)*0.25L;
        long double y=engine_math::engineMath_cos_impl(angle)*0.25L;
        const float* a=linePositions->get(i); const float* b=linePositions->get(i+1);
        float* p0=quadPositions->get(i*4); float* p1=quadPositions->get(i*4+1);
        float* p2=quadPositions->get(i*4+2); float* p3=quadPositions->get(i*4+3);
        p0[0]=static_cast<float>(a[0]-x); p0[1]=static_cast<float>(a[1]+y); p0[2]=a[2];
        p1[0]=static_cast<float>(a[0]+x); p1[1]=static_cast<float>(a[1]-y); p1[2]=a[2];
        p2[0]=static_cast<float>(b[0]-x); p2[1]=static_cast<float>(b[1]+y); p2[2]=b[2];
        p3[0]=static_cast<float>(b[0]+x); p3[1]=static_cast<float>(b[1]-y); p3[2]=b[2];
    }
    lineEntity->dirtyBSphere(); quadEntity->dirtyBSphere();
}
void PuttGuide::buildSceneGeometry(int points,std::uint32_t tick) {
    pointCount=points; tint[0]=tint[1]=tint[2]=1.0f;
    linePositions=new ssgVertexArray(points); quadPositions=new ssgVertexArray(points*4-4);
    lineColors=new ssgColourArray(); quadColors=new ssgColourArray();
    sgVec3 position={0,0,0}; sgVec4 lineColor={1,1,1,0.75f},quadColor={1,1,1,0.5f};
    for (int i=0;i<points;++i) { linePositions->add(position); lineColors->add(lineColor); }
    for (int i=0;i<points*4-4;++i) { quadPositions->add(position); quadColors->add(quadColor); }
    auto* state=new ssgSimpleState;
    state->disable(GL_TEXTURE_2D); state->disable(GL_LIGHTING); state->setShadeModel(GL_SMOOTH);
    state->disable(GL_BLEND); state->setOpaque();
    auto* line=new ssgVtxTable(GL_LINE_STRIP,linePositions,nullptr,nullptr,lineColors);
    line->setState(state); line->ref(); lineEntity=line;
    state->enable(GL_BLEND); state->setTranslucent();
    auto* quad=new ssgVtxTable(GL_QUAD_STRIP,quadPositions,nullptr,nullptr,quadColors);
    quad->setState(state); quad->ref(); quadEntity=quad;
    lastAnimationTick=tick; animationPeriod=10;
}
void PuttGuide::attachSceneRoots(ssgBranch* root,ssgBranch* secondaryRoot) {
    root->addKid(lineEntity); root->addKid(quadEntity);
    secondaryRoot->addKid(lineEntity); secondaryRoot->addKid(quadEntity);
}
void PuttGuide::hideGeometry() {
    for (int i=0;i<pointCount;++i) { float* p=linePositions->get(i); p[0]=0; p[1]=0; p[2]=-1000000.0f; }
    for (int i=0;i<pointCount*4-4;++i) { float* p=quadPositions->get(i); p[0]=0; p[1]=0; p[2]=-1000000.0f; }
    lineEntity->dirtyBSphere();
}
void PuttGuide::animateColors(std::uint32_t tick) {
    if (tick-lastAnimationTick<animationPeriod) return;
    auto advanceTint=[&]() {
        tint[2]=static_cast<float>(tint[2]+0.1); if (tint[2]>1.0) tint[2]=0.9f;
        tint[1]=static_cast<float>(tint[1]+0.05); if (tint[1]>1.0) tint[1]=0.9f;
        tint[0]=static_cast<float>(tint[0]-0.1); if (tint[0]<0.9) tint[0]=1.0f;
    };
    auto advanceAlpha=[&](bool toggle) {
        if (!alphaRising) { alpha=static_cast<float>(alpha-0.005); if (alpha<0.15) { if (toggle) alphaRising=1; alpha=0.5f; } }
        else { alpha=static_cast<float>(alpha+0.005); if (alpha>0.5) { if (toggle) alphaRising=0; alpha=0.15f; } }
    };
    auto color=[&](float* p,bool writeAlpha) { p[0]=tint[0]; p[1]=tint[2]; p[2]=tint[1]; if (writeAlpha) p[3]=alpha; };
    for (int i=0;i<pointCount*4-4;i+=4) {
        advanceTint(); advanceAlpha(false); color(quadColors->get(i),true); color(quadColors->get(i+1),true);
        advanceTint(); advanceAlpha(true); color(quadColors->get(i+2),true); color(quadColors->get(i+3),true);
    }
    for (int i=0;i<pointCount;++i) { advanceTint(); color(lineColors->get(i),false); }
    quadEntity->dirtyBSphere(); animationPeriod=10; lastAnimationTick=tick;
}
}
