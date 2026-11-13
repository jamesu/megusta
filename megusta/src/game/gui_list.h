// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "pu.h"
#include <cstdint>
namespace megusta {
class GuiVerticalSlider;
struct GuiListItemHeader { std::int32_t colorStyle; };
struct GuiActiveBar {
    int atlasId=-1,imageIndex=0,offsetX=0,offsetY=0,width=0,height=0;
    float pulseFrequency=0,pulseAmplitude=0,pulseBase=0,elapsedSeconds=0;
};
class GuiListButton : public puObject {
public:
    GuiListButton(int capacity,int textBytes,unsigned options,int x,int y,int right,int top);
    ~GuiListButton();
    void clearItems();
    void syncScrollControls();
    void selectText(const char*);
    GuiListItemHeader* getItemRecord(int);
    char* getItemText(int);
    void scrollToItem(int);
    void setItemText(int,const char*);
    void appendItem(const char*,bool scroll);
    void scrollBy(int);
    bool canScroll(int) const;
    void setColumnPosition(int,int);
    void setSelectedIndex(int);
    bool handleDirection(int,int);
    int specialCommand(const char*,const char*,int,int,int,int);
    void draw(int,int) override;
    void doHit(int,int,int,int) override;
    int capacity,textBytes,rowBytes,count=0,firstVisibleRow=0,hoveredRow=-1,selectedRow=-1;
    int visibleRows=0,lineGap=5,padLeft=8,padTop=10,padRight=10,padBottom=8,hintColumn=-1;
    unsigned options;
    bool controlsReady=false;
    unsigned char* rows;
    int columnPositions[10]{};
    float fontColors[8][5][4];
    GuiActiveBar activeBar;
    puObject *scrollUp=nullptr,*scrollDown=nullptr;
    GuiVerticalSlider* scrollbar=nullptr;
};
class GuiVerticalSlider : public puObject {
    void setRecoveredValue(long double value);
public:
    GuiVerticalSlider(int x,int y,int right,int top,int atlas,int firstImage);
    void syncListRange(int position,int count,int visible);
    int isHit(int,int) override;
    void doHit(int,int,int,int) override;
    void draw(int,int) override;
    std::int64_t specialCommand(const char*,int,int,int);
    float sliderFraction,lastCallbackValue,callbackDelta=0.1f;
    int vertical=1,callbackMode=1,atlasId,firstImage,visualState=-1;
    int visibleRows=0,itemCount=0,scrollPosition=0;
    GuiListButton* list=nullptr;
};
}
