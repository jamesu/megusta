// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "pu.h"
#include <cstdint>
class ssgRoot;
class ssgTransform;
class ssgEntity;
namespace megusta {
class GuiToggleLozenge : public puObject {
public:
    GuiToggleLozenge(int,int,int,int,unsigned,int,int,int);
    void draw(int,int) override;
    int specialCommand(const char*,int,int,int);
    int visualState=3,atlasId,firstImage,cellExtent,fontId=-1,fontSize,baselineOffset=1;
    unsigned options;
    bool latched=false;
    float fade;
};
class GuiLozenge2 : public puObject {
public:
    GuiLozenge2(int,int,int,int,unsigned);
    void draw(int,int) override;
    bool handleDirection(int,int);
    void activateCallback();
    int specialCommand(const char*,unsigned,int,int);
    int visualState=3,currentDirection=0,previousDirection=0,fontId=-1,fontSize,baselineOffset=2;
    unsigned options;
    float fade;
};
void guiLozengeActivateCallback(puObject*);
class GuiHeader : public puObject {
public:
    GuiHeader(int,int,int,int,bool);
    void draw(int,int) override;
    float animationPhase;
    std::uint32_t unresolvedAnimationWord=0;
};
class GuiText : public puObject {
public:
    GuiText(int,int);
    void draw(int,int) override;
    int specialCommand(const char*);
    bool parsed=false;
    float fade=1,primaryColor[4],secondaryColor[4];
    int fontId,fontSize,alignmentScale,drawStyle;
};
class GuiFrame : public puObject {
public:
    GuiFrame(int,int,int,int);
    void draw(int,int) override;
};
class GuiShadowBox : public puObject {
public:
    GuiShadowBox(int,int,int,int,int fadeMilliseconds);
    void setCornerColor(int,float,float,float,float);
    void draw(int,int) override;
    float fade,fadeRate,cornerColors[4][4];
    bool parseLegend=true;
};
class GuiSimpleButton : public puObject {
public:
    GuiSimpleButton(int,int,int,int);
    void draw(int,int) override;
    float fade=1,highlightPalette[5][4];
    int visualState=3;
};
class GuiStatusBar : public puObject {
public:
    GuiStatusBar(int,int,int,int,int textureIndex);
    void draw(int,int) override;
    int textureIndex;
    float displayValue=0;
};
class GuiBitmapButton : public puObject {
public:
    GuiBitmapButton(int,int);
    ~GuiBitmapButton();
    void draw(int,int) override;
    std::uint32_t bitmap=0;
    bool ownsBitmap=false;
    int width=0,height=0;
    float fade=1,fadeRate=2;
};
class GuiAnimatedBitmapButton : public puObject {
public:
    GuiAnimatedBitmapButton(int,int,int,int);
    void draw(int,int) override;
    int atlasId=0,offsetX=0,offsetY=0,width=0,height=0,frameRate=0;
    int stateFrameOffsets[5]{},stateFrameCounts[5]{};
    // The original constructor does not write this field; keep it indeterminate.
    int visualState;
    float elapsedSeconds=0,fade=1;
};
class GuiSceneButton : public puObject {
public:
    GuiSceneButton();
    ~GuiSceneButton();
    void draw(int,int) override;
    void renderPreview();
    void addSceneChild(ssgEntity*);
    int specialCommand(const char*);
    ssgRoot* sceneRoot;
    ssgTransform* previewEntity=nullptr;
    int scissor[4]{};
    bool scissorEnabled=false;
};
void flushGuiScenePreview();
}
