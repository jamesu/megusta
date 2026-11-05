#include "spatial_object.h"
#include "powerups.h"
#include "hole.h"
#include "native_game_helpers.h"
#include "engine/resource_runtime.h"
#include "engine/string_services.h"
#include "engine/opengl_adapter.h"
#include <cstring>
#include <stdexcept>
namespace megusta {
namespace {
constexpr const char spatialMapExtension[] = ".py";
constexpr const char spatialMapReadMode[] = "rt";
constexpr const char spatialMapWriteMode[] = "wt";
}
SpatialObjects gameSpatialObjects;
SpatialObject* SpatialObjects::createFromTag(const char* text) {
    const auto is=[&](const char* tag) { return !strings::compareInsensitiveBounded(text,tag,std::strlen(tag)); };
    if (is("SpatialCamera")) return new SpatialCamera;
    if (is("SpatialTarget")) return new SpatialTarget;
    if (is("SpatialRegion")) return new SpatialRegion;
    if (is("SpatialZone")) return new SpatialZone;
    if (is("SpatialRepel")) return new SpatialRepel;
    if (is("PowerUp")) return new PowerUp(powerUpsRuntime().random);
    return nullptr;
}
void SpatialObjects::insert(SpatialObject* object) {
    if (!object) return;
    for (auto* current=head;current;current=current->next_) if (object==current) return;
    if (!object->spatialId_) {
        int id=1;
        while (head) {
            auto* current=head; while (current && current->spatialId_!=id) current=current->next_;
            if (!current || ++id>999998) break;
        }
        object->spatialId_=id;
    }
    object->next_=head; head=object; lastCreated=selected=object;
}
bool SpatialObjects::loadMapFile(const char* mapName,const char* extension) {
    lastCreated=iterator=selected=head=nullptr;
    if (!extension) extension=spatialMapExtension;
    char line[256]; std::strcpy(line,"data/gobj_"); std::strncat(line,mapName,255); line[255]='\0';
    std::strncat(line,extension,255); line[255]='\0'; auto* file=sharedIo->open(line,spatialMapReadMode);
    if (!file) return false;
    line[0]='\0';
    while (sharedIo->getLine(line,255,file)) {
        auto* object=createFromTag(line);
        if (object) { object->parseRecord(line,255); insert(object); }
        line[0]='\0';
    }
    sharedIo->close(file); return true;
}
bool SpatialObjects::saveMapFile(const char* mapName,const char* extension) const {
    if (!extension) extension=spatialMapExtension;
    char text[1024]; std::strcpy(text,"data/gobj_"); std::strncat(text,mapName,1023); text[1023]='\0';
    std::strncat(text,extension,255); text[1023]='\0'; auto* file=sharedIo->open(text,spatialMapWriteMode);
    if (!file) return false;
    for (auto* object=head;object;object=object->next_) {
        const int length=object->writeRecord(text,1023); sharedIo->write(text,length,1,file);
    }
    sharedIo->close(file); return true;
}
void loadCurrentSpatialMap(const char* extension) { gameSpatialObjects.loadMapFile(gameHole.ownedInitString1,extension); }
void SpatialObjects::saveAndDestroy(const char* mapName,const char* extension) {
    saveMapFile(mapName,extension);
    while (head) {
        auto* object=head;
        for (auto* current=head;current;current=current->next_)
            current->removeLinkedObject(object);
        head=object->next_;
        if (selected==object) selected=nullptr;
        if (iterator==object) iterator=iterator->next_;
        if (lastCreated==object) lastCreated=nullptr;
        delete object;
    }
}
SpatialObject* SpatialObjects::validateSelection() {
    for (auto* object=head;object;object=object->next_) if (selected==object) return selected;
    return selected=head;
}
void SpatialObjects::drawAllDebug(const SpatialDebugServices& services) {
    if (!isEditing()) return;
    validateSelection();
    if (!services.draw) throw std::logic_error("Spatial debug draw targets are not assembled");
    services.gl.disable(0x10);
    for (iterator=head;iterator;iterator=iterator->next_) services.draw(services.context,*iterator);
    services.gl.enable(0x10);
}
void SpatialObjects::drawSelectedDebug(const SpatialDebugServices& services) {
    if (isEditing() && validateSelection()) {
        if (!services.selectedInfo) throw std::logic_error("Spatial selected-info targets are not assembled");
        services.selectedInfo(services.context,*selected);
    }
}
}
