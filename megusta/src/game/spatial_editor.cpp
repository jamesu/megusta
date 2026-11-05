#include "spatial_editor.h"
#include "spatial_object.h"
#include "powerups.h"
#include "balls.h"
#include "putter.h"
#include "scene_grid.h"
#include "gui_runtime.h"
#include "gameplay_messages.h"
#include "engine/engine_math.h"
#include "engine/string_services.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace megusta {
void showWireSelection(const WireDebugState& state,WireDebugServices& services) {
    if (!services.modeNames) throw std::logic_error("Wire mode-name table is not bound");
    char text[512]; const auto* name=services.modeNames[state.mode];
    if (!state.mode) { setGuiTimedMessage(name,90); return; }
    if (state.mode==1) std::sprintf(text,"WireMode= %s, Object= %s",name,services.objects.objects[state.selectedObjectIndex].name);
    else if (state.mode==2) {
        auto& object=services.objects.objects[state.selectedObjectIndex]; Point3F vertices[3];
        object.triangles[state.selectedTriangleIndex].copyVertices(vertices,services.objects.triangleServices());
        std::sprintf(text,"%s/%d/%d: (%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f)",object.name,state.selectedObjectIndex,state.selectedTriangleIndex,
            static_cast<double>(vertices[0].x),static_cast<double>(vertices[0].y),static_cast<double>(vertices[0].z),
            static_cast<double>(vertices[1].x),static_cast<double>(vertices[1].y),static_cast<double>(vertices[1].z),
            static_cast<double>(vertices[2].x),static_cast<double>(vertices[2].y),static_cast<double>(vertices[2].z));
        setGuiTimedMessage(text,30000); return;
    } else std::sprintf(text,"WireMode= %s",name);
    setGuiTimedMessage(text,90);
}
void handleWireInput(WireDebugState& state,WireDebugServices& services) {
    if (!services.debugKeys || services.networkInGame || !services.input.keyCode) return;
    const bool reverse=(services.input.modifiers&4)!=0;
    switch (services.input.keyCode) {
    case 0x106: state.mode+=reverse?-1:1; if (state.mode<0) state.mode=6; else if (state.mode>6) state.mode=0; break;
    case 0x107:
        state.mode=1; state.selectedObjectIndex+=reverse?-1:1;
        if (state.selectedObjectIndex<1) state.selectedObjectIndex=services.objects.objectCount;
        else if (state.selectedObjectIndex>services.objects.objectCount) state.selectedObjectIndex=1;
        break;
    case 0x108:
        state.mode=2; state.selectedTriangleIndex+=reverse?-1:1;
        if (state.selectedTriangleIndex<0) state.selectedTriangleIndex=services.objects.objects[state.selectedObjectIndex].triangleCount;
        else if (state.selectedTriangleIndex>=services.objects.objects[state.selectedObjectIndex].triangleCount) state.selectedTriangleIndex=0;
        break;
    default: return;
    }
    showWireSelection(state,services); state.applySceneCallbacks(services.objects,services.grid,services.allCallback,services.triangleCallback,services.candidatesCallback);
}
void SpatialEditor::showHelp() {
    // Text retained by string_xrefs.tsv; original line separators were flattened
    // by that exporter, so diagnostic layout remains comparison pending.
    showSpatialEditorMessage("help! ==== keypad moves the ball location numlock keypad moves most recent object 9 and 3 move object up and down + and - expand or shrink object home key cycles through object sub-type   "
        "other keys (with alt pressed) ========== h  = help m  = move current to wherever camera is u  = unadd current if any r  = rotate (only applys to bounding boxes) "
        "1  = make ai target 2  = make region 3  = make zone 4  = make repel 9  = make camera p  = make powerup    = remember target source or wire target to destination "
        "v  = visit next - this changes the current object b  = wind backwards to previous - change current object wasd = forward/sidestep left/backward/sidestep right "
        "Comments ======== Place AI targets with (1) as places for AI to aim for. Place regions with (2) as places to aim from. Connect targets and regions by hitting alt-space over each "
        "Place cameras with (9) and change camera type with (home) Trigger cameras with regions (2) or zones (3) Create repel zones with (4) and change sub-type with (home) Make sure you have level files checked out if editing ");
}
void SpatialEditor::processInput(SpatialEditorServices& s) {
    auto& frame=s.cameraFrame; auto& input=frame.input; const unsigned key=input.keyCode;
    const bool alternate=(input.modifiers&1)!=0; const unsigned command=alternate?key:0,edges=input.buttons&~input.previousButtons;
    if (!s.debugKeys || s.networkInGame) return;
    if (command=='x' || (edges&0x100)) {
        s.editorMode=1-s.editorMode; auto* ball=frame.players.currentPlayer->getBall(frame.balls); ball->flags|=0x20;
        if (!s.editorMode) { ball->flags&=~0x20u; frame.runtime.transitionVector={0,0,0}; }
        else frame.runtime.transitionVector={ball->position[0],ball->position[1],ball->position[2]};
    }
    if (!s.editorMode) return;
    const char character=static_cast<char>(key); const unsigned buttons=input.buttons; const bool vertical=(buttons&0x80)!=0;
    if (character=='f') { if (s.moveStep==1) { showSpatialEditorMessage("moving fast"); s.moveStep=5; } else { showSpatialEditorMessage("moving slow"); s.moveStep=1; } }
    if (vertical) s.moveStep=1;
    const float z=frame.putter.smoothedAim[2];
    const float angle=static_cast<float>(engine_math::engineMath_atan2_impl(-frame.putter.smoothedAim[1],-frame.putter.smoothedAim[0]));
    auto& camera=s.camera; auto& cursor=frame.runtime.transitionVector;
    if ((buttons&4) || character==',' || character=='<') { if (!vertical) camera.headingDegrees-=2; else { camera.pitchDegrees-=3; camera.clampEditorOrbit(isEditing); } camera.mouseCaptured=0; }
    if ((buttons&8) || character=='.' || character=='>') { if (!vertical) camera.headingDegrees+=2; else { camera.pitchDegrees+=3; camera.clampEditorOrbit(isEditing); } camera.mouseCaptured=0; }
    if (character=='[') { camera.followDistance-=5; camera.clampEditorOrbit(isEditing); } else if (character==']') { camera.followDistance+=5; camera.clampEditorOrbit(isEditing); }
    const auto forward=[&](int direction) { const float dx=static_cast<float>(engine_math::engineMath_cos_impl(angle)*direction*s.moveStep);
        cursor.x+=dx; cursor.y=static_cast<float>(static_cast<long double>(cursor.y)+engine_math::engineMath_sin_impl(angle)*direction*s.moveStep); cursor.z+=z; };
    const auto sideways=[&](int direction) { cursor.x+=static_cast<float>(-engine_math::engineMath_sin_impl(angle)*direction*s.moveStep);
        cursor.y=static_cast<float>(static_cast<long double>(cursor.y)+engine_math::engineMath_cos_impl(angle)*direction*s.moveStep); cursor.z+=z; };
    if (((buttons&0x1000) && vertical) || character=='h') cursor.z+=s.moveStep; else if ((buttons&0x1000) || character=='w') forward(1);
    if (((buttons&0x4000) && vertical) || character=='i') cursor.z-=s.moveStep; else if ((buttons&0x4000) || character=='s') forward(-1);
    if ((buttons&0x8000) || character=='a') sideways(1); if ((buttons&0x2000) || character=='d') sideways(-1);
    auto& objects=frame.spatial; auto* moving=objects.validateSelection();
    const auto accepts=[&](SpatialObject* object) {
        if (!object) return false; const char* tag=object->typeTag();
        if (frame.typeFilter==2) return !strings::compareInsensitive(tag,"SpatialRepel");
        if (frame.typeFilter==3) return !strings::compareInsensitive(tag,"SpatialRegion") || !strings::compareInsensitive(tag,"SpatialTarget");
        if (frame.typeFilter==5) return !strings::compareInsensitive(tag,"SpatialRegion") || !strings::compareInsensitive(tag,"SpatialCamera") || !strings::compareInsensitive(tag,"SpatialZone");
        return true;
    };
    const bool previous=(edges&2) || character=='v' || character=='V';
    const bool next=!previous && ((edges&1) || character=='b' || character=='B');
    if (previous || next) {
        showSpatialEditorMessage(previous?"V = examine previous":"W = examine next");
        for (int attempt=0;attempt<100;++attempt) {
            if (next) { objects.selected=objects.selected?objects.selected->next_:objects.head; if (!objects.selected) objects.selected=objects.head; }
            else {
                auto* before=objects.head;
                while (before && before->next_ && before->next_!=objects.selected) before=before->next_;
                objects.selected=before;
            }
            if (!objects.selected || accepts(objects.selected)) break;
        }
        if (objects.selected) { const float* p=objects.selected->position(); cursor={p[0],p[1],p[2]}; showSpatialEditorMessage(objects.selected->typeTag()); }
    }
    if (moving && !alternate) {
        const auto move=[&](int direction,bool side) {
            const float x=static_cast<float>((side?-engine_math::engineMath_sin_impl(angle):engine_math::engineMath_cos_impl(angle))*direction*s.moveStep);
            const float y=static_cast<float>((side?engine_math::engineMath_cos_impl(angle):engine_math::engineMath_sin_impl(angle))*direction*s.moveStep);
            moving->translate(x,y,z);
        };
        if ((buttons&0x100000) || character=='8') move(1,false); else if ((buttons&0x200000) || character=='2') move(-1,false);
        if ((buttons&0x400000) || character=='4') move(1,true);
        else if (character=='5') {
            showSpatialEditorMessage("Move to cursor"); moving=objects.validateSelection();
            if (moving) { const float* p=moving->position(); camera.setLookat(frame.runtime,frame.fixedTick,p[0],p[1],p[2],2);
                camera.setPosition(frame.runtime,frame.fixedTick,p[0]-40,p[1]-40,p[2]); camera.restartFollow(frame.runtime); }
        } else if ((buttons&0x800000) || character=='6') move(-1,true);
        if ((buttons&0x10000) || character=='9') moving->translate(0,0,static_cast<float>(s.moveStep));
        else if ((buttons&0x20000) || character=='3') moving->translate(0,0,static_cast<float>(-s.moveStep));
        const auto resize=[&](float amount) { if (dynamic_cast<PowerUp*>(moving)) moving->adjustRadiusLimited(amount); else moving->adjustRadius(amount); };
        if ((buttons&0x40000) || character=='-') resize(static_cast<float>(-s.moveStep)); else if ((buttons&0x80000) || character=='+') resize(static_cast<float>(s.moveStep));
        if ((edges&0x10) || character=='j') {
            if (auto* repel=dynamic_cast<SpatialRepel*>(moving)) repel->cycleConstraintKind(showSpatialEditorMessage);
            else if (auto* spatialCamera=dynamic_cast<SpatialCamera*>(moving)) spatialCamera->cycleMode(showSpatialEditorMessage);
            else if (auto* power=dynamic_cast<PowerUp*>(moving)) power->cycleDefinition();
        }
    }
    if (++repeatCounter<10) return; if (character || buttons) repeatCounter=0;
    const auto remove=[&](SpatialObject* object) {
        for (auto* current=objects.head;current;current=current->next_) current->removeLinkedObject(object);
        if (objects.head==object) objects.head=object->next_;
        else for (auto* current=objects.head;current;current=current->next_) if (current->next_==object) { current->next_=object->next_; break; }
        if (objects.selected==object) objects.selected=nullptr; if (objects.iterator==object) objects.iterator=object->next_; if (objects.lastCreated==object) objects.lastCreated=nullptr; delete object;
    };
    switch (command) {
    case ' ': {
        bool found=false; for (auto* current=objects.head;current;current=current->next_) if (current==relationshipSource) found=true;
        if (!found) relationshipSource=nullptr;
        if (moving) {
            if (relationshipSource && relationshipSource!=moving && !std::strcmp(moving->typeTag(),"SpatialRegion")) {
                showSpatialEditorMessage("Trying to add relationship target"); static_cast<SpatialRegion*>(moving)->addLinkedId(relationshipSource);
            } else if (relationshipSource) showSpatialEditorMessage("Remembering relationship source");
            relationshipSource=moving;
        }
        break;
    }
    case 'h': showHelp(); break;
    case 'm':
        showSpatialEditorMessage("m = move thing to camera"); moving=objects.validateSelection();
        if (moving) { sgMakeIdentMat4(moving->transform_); moving->transform_[3][0]=camera.controlEye.x; moving->transform_[3][1]=camera.controlEye.y; moving->transform_[3][2]=camera.controlEye.z;
            if (auto* spatialCamera=dynamic_cast<SpatialCamera*>(moving)) { spatialCamera->lookAt_[0]=camera.controlLookAt.x; spatialCamera->lookAt_[1]=camera.controlLookAt.y; spatialCamera->lookAt_[2]=camera.controlLookAt.z; } }
        break;
    case 'r': if (auto* repel=dynamic_cast<SpatialRepel*>(objects.validateSelection())) repel->addEulerRotation(static_cast<float>(s.moveStep),0,0); break;
    case 'u': if (auto* object=objects.validateSelection()) remove(object); break;
    case 'v': showSpatialEditorMessage("V = Normal View.."); camera.setState(frame.runtime,CameraMode::Automatic); break;
    case 'z': showSpatialEditorMessage("Z = zoom object radius"); if (auto* object=objects.validateSelection()) { if (dynamic_cast<PowerUp*>(object)) object->adjustRadiusLimited(static_cast<float>(s.moveStep)); else object->adjustRadius(static_cast<float>(s.moveStep)); } break;
    }
    const char* tag=nullptr; const char* message=nullptr;
    switch (command) {
    case '1': tag="SpatialTarget"; message="Making target"; break; case '2': tag="SpatialRegion"; message="Making region"; break;
    case '3': tag="SpatialZone"; message="Making zone"; break; case '4': tag="SpatialRepel"; message="Making repel"; break;
    case '9': tag="SpatialCamera"; message="Making camera"; break; case 'p': tag="PowerUp"; message="Making PowerUp"; break;
    }
    if (tag) {
        showSpatialEditorMessage(message); auto* created=SpatialObjects::createFromTag(tag); objects.insert(created);
        if (auto* power=dynamic_cast<PowerUp*>(created)) s.powers.attachPowerUpRecord(power);
        sgMakeIdentMat4(created->transform_); created->transform_[3][0]=cursor.x; created->transform_[3][1]=cursor.y; created->transform_[3][2]=cursor.z;
        if (command!='p') objects.saveMapFile(s.mapName,s.spatialExtension);
    }
    if ((edges&0x20) || character=='*') {
        showSpatialEditorMessage("Making PowerUp"); auto* power=static_cast<PowerUp*>(SpatialObjects::createFromTag("PowerUp"));
        objects.insert(power); s.powers.attachPowerUpRecord(power); power->translate(cursor.x,cursor.y,cursor.z);
    }
    if ((edges&0x200) || character=='n' || character=='N') { const float p[3]={cursor.x,cursor.y,cursor.z}; frame.players.currentPlayer->getBall(frame.balls)->setLocation(p); }
}
}
