// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
class puObject;
class ssgRoot;
namespace megusta {
class GameShapeAdapter;
struct GuiDebugState;
struct GuiBindingNode { std::uint32_t code; char* action; GuiBindingNode* next; };
struct GuiShapePreviewRecord {
    bool active=false,clearBackground=false;
    char displayName[10]{};
    GameShapeAdapter* shapeAdapter=nullptr;
    float location[3]{},rotationPerSecond[3]{},rotation[3]{},eye[3]{},target[3]{};
    int screen[4]{},backgroundAtlas=0,backgroundBitmap=0,backgroundRect[4]{};
    float backgroundColour[3]{};
};
class guiBindings {
public:
    void initialize();
    void release();
    void add(const char* name,const char* action);
    void remove(const char* name);
    void dispatchInput();
    int level=0;
    std::uint32_t buttonMasks[5]{};
    GuiBindingNode* buttonActions[5]{};
    GuiBindingNode* keyActions[5]{};
    void clear(); void push(); void pop();
    void addCode(std::uint32_t,const char*,bool button);
    void removeCode(std::uint32_t,bool button);
};
class guiElement {
public:
    puObject* widget=nullptr;
    int type=0,x=0,y=0,width=0,height=0,tabState=0;
    std::uint32_t flags=0;
    char* attributes[6]{};
    char* eventHandlers[3]{};
    int getWidth() const; int getHeight() const;
    guiElement* initializeFromType(int type,int x,int y,int width,int height,unsigned option0,unsigned option1,unsigned option2,int option3);
    void setWriteOnlyAttr(int,const char*);
    int specialCmd(const char*,const char*,int,int,int,int);
    const char* getAttr(int) const;
    void setAttr(int,const char*);
    void setEH(int,const char*);
    void setPosition(int,int);
    void hide(); void show(); void release();
};
class guiSystem {
public:
    std::uint32_t* initialize();
    void refreshAnimationFlag();
    bool isWidgetInputAllowed(puObject*) const;
    void updateFrameDelta();
    void updateInteraction(bool debugKeys,bool networkInGame,GuiDebugState&);
    ssgRoot* getRoot();
    void playZoom(const char*,std::uint32_t);
    void resetZoomAnimations();
    void releaseZoomCache();
    void advanceZoomAnimations();
    void drawRoot();
    void advanceShellBackground();
    void drawShapePreviews();
    void drawShapePreviewBackgrounds();
    void updateShapePreviews();
    void drawShellBackground();
    void setBackground(const char*);
    void clearScreen();
    int shellCmd(const char*,int,int,int,int,int);
    std::uint32_t backgroundTexture=0;
    void* backgroundTextures=nullptr;
    float frameDelta=0;
    bool warpCursorOnTab=false;
    guiElement* elements[256]{};
    int elementCount=0;
    guiElement *currentElement=nullptr,*tabElement=nullptr,*pressedElement=nullptr;
    bool hasTabElements=false,hasInterface=false;
    int tooltipAreas[4][4]{};
    int tooltipDelay=0;
    float tooltipColor[4]{};
    bool tooltipDirty=false;
    GuiShapePreviewRecord shapePreviews[4]{};
    bool shapePreviewPoolAllocated=false;
    GameShapeAdapter* extraShapePreview=nullptr;
    ssgRoot* previewRoot=nullptr;
    guiBindings bindings;
    guiElement* findElement(const char*) const;
    void loadShapePreview(int,const char*,unsigned,unsigned);
    void setShapeSkin(const char*,int);
    int shapeCommand(int,const char*,int,int,int,int);
    void flushShapePreviews();
    guiElement* addElement(int type,int x,int y,int width,int height,
        unsigned option0=0,unsigned option1=0,unsigned option2=0,int option3=0);
    void removeElement(guiElement*);
    void setCurrentTab(guiElement*,bool warp);
    guiElement* findNearestTab(int,int,bool playSound=true);
    guiElement* findMouseOver(int,int,int) const;
};
}
