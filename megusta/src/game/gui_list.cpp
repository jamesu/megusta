#include "gui_list.h"
#include "gui_runtime.h"
#include "gui_render.h"
#include "engine/opengl_adapter.h"
#include "engine/runtime_adapters.h"
#include "engine/string_services.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace megusta {
namespace {
long double clampSlider(long double value) {
    if (value>=0.0f) { if (value>1.0f) value=1.0f; }
    else value=0.0f;
    return value;
}
std::int64_t roundLikeRecoveredFtol(long double value) {
    // The saved MSVC helper rounds x87 values to an integer in the game's
    // normal round-to-nearest mode. These conversions are bounded to the UI range.
    return static_cast<std::int64_t>(std::nearbyint(value));
}
void deactivateOtherWidget(puObject* self) {
    auto* active=puActiveWidget();
    if (active && active!=self) { active->invokeDownCallback(); puDeactivateWidget(); }
}
}
GuiListButton::GuiListButton(int size,int textSize,unsigned flags,int x,int y,int right,int top)
    :puObject(x,y,right,top),capacity(size),textBytes(textSize),rowBytes(textSize+4),options(flags) {
    type|=0xc0000040u;
    const float colors[5][4]={{0.5f,0.5f,0.8f,1},{0.9f,0.9f,0.7f,1},
        {1,1,1,1},{1,1,1,0.5f},{0,0,0,0.5f}};
    for (auto& style:fontColors) std::memcpy(style,colors,sizeof(colors));
    rows=static_cast<unsigned char*>(std::malloc(capacity*rowBytes));
    if (!rows) engine_runtime::engineRuntime_fatalExit_impl();
    clearItems();
}
GuiListButton::~GuiListButton() { if (rows) std::free(rows); }
void GuiListButton::clearItems() {
    std::memset(rows,0,rowBytes*capacity);
    count=0; firstVisibleRow=0; hoveredRow=-1; setSelectedIndex(-1);
}
void GuiListButton::syncScrollControls() {
    if (!visibleRows) return;
    if (scrollUp) { if (canScroll(-1)) scrollUp->activate(); else scrollUp->greyOut(); }
    if (scrollDown) { if (canScroll(1)) scrollDown->activate(); else scrollDown->greyOut(); }
    if (scrollbar) scrollbar->syncListRange(firstVisibleRow,count,visibleRows);
}
void GuiListButton::selectText(const char* text) {
    if (rows) for (int i=0;i<count;++i) if (!std::strcmp(getItemText(i),text)) {
        setSelectedIndex(i); return;
    }
}
GuiListItemHeader* GuiListButton::getItemRecord(int index) {
    if (!rows || index<0 || index>=capacity) engine_runtime::engineRuntime_fatalExit_impl();
    return reinterpret_cast<GuiListItemHeader*>(rows+rowBytes*index);
}
char* GuiListButton::getItemText(int index) { return reinterpret_cast<char*>(getItemRecord(index)+1); }
void GuiListButton::scrollToItem(int index) {
    if (index<0) index=0;
    if (index>=count) index=count-1;
    if (visibleRows) {
        firstVisibleRow=0;
        while (firstVisibleRow+visibleRows<=index) ++firstVisibleRow;
        syncScrollControls();
    }
}
void GuiListButton::setItemText(int index,const char* text) {
    if (!rows) engine_runtime::engineRuntime_fatalExit_impl();
    char* destination=getItemText(index);
    std::strncpy(destination,text,textBytes-1); destination[textBytes-1]='\0';
}
void GuiListButton::appendItem(const char* text,bool scroll) {
    if (count<capacity) { const int index=count++; setItemText(index,text); }
    else {
        // The original copy overlaps. memmove preserves the intended left shift.
        std::memmove(getItemRecord(0),getItemRecord(1),(capacity-1)*rowBytes);
        setItemText(capacity-1,text); hoveredRow=-1;
        if (selectedRow>=0) setSelectedIndex(selectedRow-1);
    }
    if (scroll) scrollToItem(count);
    syncScrollControls();
}
void GuiListButton::scrollBy(int delta) {
    firstVisibleRow+=delta;
    if (firstVisibleRow>count-1) firstVisibleRow=count-1;
    if (firstVisibleRow<0) firstVisibleRow=0;
    syncScrollControls();
}
bool GuiListButton::canScroll(int direction) const {
    if (!direction) return true;
    return direction<0 ? firstVisibleRow>0 : firstVisibleRow+visibleRows<count;
}
void GuiListButton::setColumnPosition(int column,int position) {
    if (column>=0 && column<10) columnPositions[column]=position;
    options|=2;
}
void GuiListButton::setSelectedIndex(int index) {
    selectedRow=index;
    if (index>=count) selectedRow=-1;
    setValue(selectedRow); puRefresh=TRUE; syncScrollControls();
}
bool GuiListButton::handleDirection(int horizontal,int vertical) {
    if (horizontal || (options&1)) return false;
    int index=selectedRow;
    if (index<0) index=0;
    else if (vertical) {
        if (vertical>=1 && index>=1) --index;
        else if (vertical<=-1 && index<count-1) ++index;
        else return false;
    }
    if (index<0) return false;
    setSelectedIndex(index); scrollToItem(index);
    auto& runtime=*guiRuntime;
    const int lineHeight=legendFont.getStringHeight()+lineGap;
    runtime.input.mouse.y=runtime.screenHeight-((lineHeight*-40)/100-
        (index-firstVisibleRow)*lineHeight+abox.max[1]);
    runtime.input.mouse.x=(abox.max[0]+abox.min[0])/2;
    runtime.warpCursor(runtime.context,runtime.input.mouse.x,runtime.input.mouse.y);
    if (vertical) dispatchGuiEvent(this,0);
    return true;
}
void GuiListButton::doHit(int button,int edge,int,int y) {
    deactivateOtherWidget(this);
    if (!button && !(options&1)) {
        if (edge==active_mouse_edge || active_mouse_edge==0xfe) {
            lowlight();
            int index=(abox.max[1]-lineGap-y)/(legendFont.getStringHeight()+lineGap)+firstVisibleRow;
            if (index<0) index=0;
            else if (index>=count) index=count-1;
            setSelectedIndex(index); invokeCallback();
        } else highlight();
    } else lowlight();
}
int GuiListButton::specialCommand(const char* command,const char* text,int a,int b,int c,int d) {
    const auto is=[&](const char* value) { return !strings::compareInsensitive(command,value); };
    if (is("add")) appendItem(text,a!=0);
    else if (is("clear")) clearItems();
    else if (is("scroll")) scrollBy(a);
    else if (is("set")) setItemText(a,text);
    else if (is("select")) selectText(text);
    else if (is("GetIndexSelected") || is("GetSelected")) return selectedRow;
    else if (is("GetIndexTop")) return firstVisibleRow;
    else if (is("GetRowsVis")) return visibleRows;
    else if (is("GetRowsUsed")) return count;
    else if (is("GetRowsAllocated")) return capacity;
    else if (is("GetRowsAvail")) return capacity-count;
    else if (is("SetColumnOffset")) setColumnPosition(a,b);
    else if (is("SetFont")) { legendFont=puFont(guiRuntime->resolveFont(guiRuntime->context,a),b,0); lineGap=c; }
    else if (is("SetFontColors")) {
        if (a<0 || a>7 || b<0 || b>4) engine_runtime::engineRuntime_fatalExit_impl();
        auto* color=fontColors[a][b]; std::sscanf(text,"%f,%f,%f,%f",color,color+1,color+2,color+3);
    } else if (is("SetRowColor")) getItemRecord(a)->colorStyle=b;
    else if (is("SetHintColumn")) hintColumn=a;
    else if (is("SetBoxPad")) { padLeft=a; padTop=b; padRight=c; padBottom=d; }
    else if (is("SetIndexSelected")) setSelectedIndex(a);
    else if (is("ActiveBar")) std::sscanf(text,"%d,%d, %d,%d,%d,%d, %f,%f,%f",
        &activeBar.atlasId,&activeBar.imageIndex,&activeBar.offsetX,&activeBar.offsetY,
        &activeBar.width,&activeBar.height,&activeBar.pulseFrequency,&activeBar.pulseAmplitude,&activeBar.pulseBase);
    return 0;
}
void GuiListButton::draw(int dx,int dy) {
    auto& runtime=*guiRuntime;
    if (!controlsReady) {
        controlsReady=true;
        auto* owner=static_cast<guiElement*>(getUserData());
        const char* name=owner->getAttr(0);
        if (name && *name) {
            const std::string prefix=name;
            if (auto* element=runtime.system.findElement((prefix+"_UP").c_str())) scrollUp=element->widget;
            if (auto* element=runtime.system.findElement((prefix+"_DN").c_str())) scrollDown=element->widget;
            if (auto* element=runtime.system.findElement((prefix+"_SCROLL").c_str())) {
                scrollbar=static_cast<GuiVerticalSlider*>(element->widget);
                if (!scrollbar) engine_runtime::engineRuntime_fatalExit_impl();
                scrollbar->list=this;
            }
        }
    }
    if (!visible || window!=puGetWindow()) return;
    auto& renderer=*runtime.renderer;
    auto& gl=renderer.gl;
    const bool focused=runtime.system.tabElement && runtime.system.tabElement->widget==this;
    if (!(options&4)) renderer.drawNineSliceFrame(abox.min[0]-padLeft+dx,abox.min[1]-padTop+dy,
        abox.max[0]+padRight+dx,abox.max[1]+padBottom+dy,focused);
    if (!active) gl.color4f(colour[4][0],colour[4][1],colour[4][2],colour[4][3]*0.5f);
    else gl.color4fv(colour[4]);
    const int fontHeight=legendFont.getStringHeight(),lineHeight=fontHeight+lineGap;
    const int previousRows=visibleRows;
    visibleRows=((abox.max[1]-abox.min[1]+1)-lineGap)/lineHeight;
    if (!previousRows && visibleRows) syncScrollControls();
    int hovered=-1;
    const int mouseY=runtime.screenHeight-runtime.input.mouse.y;
    if (!(options&1) && isHit(runtime.input.mouse.x,mouseY) && runtime.system.isWidgetInputAllowed(this))
        hovered=((abox.max[1]-mouseY)-lineGap)/lineHeight+firstVisibleRow;
    activeBar.elapsedSeconds+=runtime.system.frameDelta;
    if (focused && hovered>=0 && activeBar.atlasId>=0) {
        float alpha=1;
        if (activeBar.pulseFrequency>0) alpha=(activeBar.pulseBase+activeBar.pulseAmplitude)*0.5L+
            (activeBar.pulseBase-activeBar.pulseAmplitude)*0.5L*
            std::cos(static_cast<long double>(activeBar.elapsedSeconds*activeBar.pulseFrequency));
        renderer.beginTexturedAlpha(alpha);
        renderer.drawTextureRect(renderer.atlases.resolve(activeBar.atlasId,activeBar.imageIndex),
            activeBar.offsetX+abox.min[0]+dx,activeBar.offsetY-(hovered-firstVisibleRow)*lineHeight-
            fontHeight+abox.max[1]+dy,activeBar.width,activeBar.height,false);
        renderer.endTexturedAlpha();
    }
    for (int row=firstVisibleRow;row<count && row<firstVisibleRow+visibleRows;++row) {
        const int style=getItemRecord(row)->colorStyle;
        int visual=0;
        if (row==selectedRow) visual=2;
        else if (row==hovered) { visual=1; hoveredRow=row; }
        char* text=getItemText(row);
        const int x=abox.min[0]+5+dx,y=abox.max[1]-(row-firstVisibleRow)*lineHeight-fontHeight+dy;
        const auto drawText=[&](const char* value,int offset) {
            gl.color4fv(fontColors[style][4]); legendFont.drawString(value,x+offset+1,y-1);
            gl.color4fv(fontColors[style][visual]); legendFont.drawString(value,x+offset,y);
        };
        if (!(options&2)) drawText(text,0);
        else for (int column=0;column<10;++column) {
            char* tab=std::strchr(text,'\t');
            if (tab) *tab='\0';
            drawText(text,columnPositions[column]);
            if (!tab) break;
            *tab='\t'; text=tab+1;
        }
    }
    draw_label(dx,dy);
}
GuiVerticalSlider::GuiVerticalSlider(int x,int y,int right,int top,int atlas,int image)
    :puObject(x,y,right,top),atlasId(atlas<0?0:atlas),firstImage(image<0?0:image) {
    type|=0x2000; lastCallbackValue=getFloatValue();
    sliderFraction=16.0f/(abox.max[1]-abox.min[1]-2);
}
void GuiVerticalSlider::syncListRange(int position,int count,int visibleCount) {
    if (visibleCount<count) {
        activate();
        setRecoveredValue(clampSlider(static_cast<long double>(position)/(count-visibleCount)));
    }
    else greyOut();
}
void GuiVerticalSlider::setRecoveredValue(long double value) {
    integer=static_cast<int>(roundLikeRecoveredFtol(value));
    floater=static_cast<float>(value);
    std::snprintf(string,sizeof(string),"%g",static_cast<double>(value));
    update_res();
    puRefresh=TRUE;
}
int GuiVerticalSlider::isHit(int x,int y) {
    if (guiRuntime->system.pressedElement==getUserData()) return 1;
    return puObject::isHit(x,y);
}
void GuiVerticalSlider::doHit(int button,int edge,int x,int y) {
    deactivateOtherWidget(this);
    if (button) return;
    guiRuntime->system.pressedElement=edge==0xff ? static_cast<guiElement*>(getUserData()) : nullptr;
    const int minimum=abox.min[vertical],extent=abox.max[vertical]-minimum;
    float value=0.5f;
    if (extent) value=clampSlider(((vertical?y:x)-minimum-extent*sliderFraction*0.5f)/
        ((1.0f-sliderFraction)*extent));
    value=1.0f-value; setRecoveredValue(value);
    if (list) list->scrollBy(static_cast<int>(roundLikeRecoveredFtol(
        (list->count-list->visibleRows)*static_cast<long double>(value)))-list->firstVisibleRow);
    if (!callbackMode) { if (edge!=active_mouse_edge) return; }
    else if (callbackMode==2 && std::fabs(lastCallbackValue-value)<callbackDelta) return;
    lastCallbackValue=value; invokeCallback(); puSetActiveWidget(this);
}
std::int64_t GuiVerticalSlider::specialCommand(const char* command,int a,int b,int c) {
    if (!strings::compareInsensitive(command,"GetIndexTop")) {
        auto index=roundLikeRecoveredFtol(
            (itemCount-visibleRows)*static_cast<long double>(clampSlider(getFloatValue())));
        scrollPosition=static_cast<int>(index); return index;
    }
    if (!strings::compareInsensitive(command,"SetIndexTop")) {
        scrollPosition=a; syncListRange(a,itemCount,visibleRows);
    } else if (!strings::compareInsensitive(command,"SetInfo")) {
        scrollPosition=a; itemCount=b; visibleRows=c; syncListRange(a,b,c);
    }
    return 0;
}
void GuiVerticalSlider::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto& runtime=*guiRuntime; auto& renderer=*runtime.renderer;
    const int x=abox.min[0]+dx,top=abox.max[1]+dy,bottom=abox.min[1]+dy,height=top-bottom;
    renderer.beginTexturedAlpha(active?1.0f:0.4f);
    const auto drawImage=[&](int image,int px,int py,int width,int height,bool state) {
        renderer.drawTextureRect(renderer.atlases.resolve(atlasId,image),px,py,width,height,state);
    };
    drawImage(firstImage,x,top,16,16,false);
    drawImage(firstImage+1,x,top-16,16,height-32,false);
    drawImage(firstImage+2,x,bottom+16,16,16,false);
    renderer.endTexturedAlpha();
    if (!active) return;
    const bool hovered=isHit(runtime.input.mouse.x,runtime.screenHeight-runtime.input.mouse.y) && runtime.system.isWidgetInputAllowed(this);
    int visual=0;
    if (!highlighted) {
        if (hovered) { visual=1; if (!visualState) playGuiAcceptSound(); }
    } else { visual=2; if (visualState!=2) playGuiDisabledSound(this); }
    visualState=visual;
    const long double value=clampSlider(getFloatValue());
    const int knobY=static_cast<int>(roundLikeRecoveredFtol(
        top-(1.0L-sliderFraction)*height*value-1.0L))+8;
    drawImage(firstImage+3+visual,x-8,knobY,32,32,true);
}
}
