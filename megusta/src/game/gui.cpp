#include "gui.h"
#include "gui_list.h"
#include "gui_widgets.h"
#include "gui_runtime.h"
#include "gui_tooltip.h"
#include "gui_file_picker.h"
#include "engine/runtime_adapters.h"
#include "engine/string_services.h"
#include "pu.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace megusta {
namespace {
// Type IDs 7 and 8 are the executable's deliberately minimal PUI text-like
// objects: puObject at a zero-size box with PUCLASS_TEXT set, but with the
// base object dispatch.  The vendored PLIB header makes puObject abstract, so
// this concrete compatibility shell supplies its otherwise-unused draw slot.
class GuiBareTextObject final : public puObject {
public:
    GuiBareTextObject(int x,int y) : puObject(x,y,x,y) { type |= PUCLASS_TEXT; }
    void draw(int,int) override {}
};

// The executable's type-0 GUI element is a live PUI interface tagged as both
// popup and dialog. Keep the vendored puInterface layout and stack behavior,
// while restoring the two additional class bits written by 0040bc20.
class GuiRootInterface final : public puInterface {
public:
    GuiRootInterface(int x,int y) : puInterface(x,y) {
        type |= PUCLASS_POPUP | PUCLASS_DIALOGBOX;
    }
};

#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(puObject)==432,
              "PUI puObject must match the executable's 32-bit base layout");
static_assert(sizeof(puGroup)==440,"PUI group must match the executable layout");
static_assert(sizeof(puInput)==448,"PUI input must match the executable layout");
#endif

void freeList(GuiBindingNode* node) { while (node) { auto* next=node->next; std::free(node->action); delete node; node=next; } }
GuiBindingNode* insert(GuiBindingNode* head,std::uint32_t code,const char* action) {
    if (!code || !action) return head;
    for (auto* node=head;node;node=node->next) if (node->code==code) {
        std::free(node->action); node->action=strings::duplicate(action); return head;
    }
    return new GuiBindingNode{code,strings::duplicate(action),head};
}
GuiBindingNode* clone(GuiBindingNode* node) {
    GuiBindingNode* result=nullptr;
    for (;node;node=node->next) result=insert(result,node->code,node->action);
    return result;
}
GuiBindingNode* removeBindingNode(GuiBindingNode* head,std::uint32_t code) {
    auto** link=&head;
    while (*link) {
        if ((*link)->code==code) { auto* old=*link; *link=old->next; std::free(old->action); delete old; break; }
        link=&(*link)->next;
    }
    return head;
}
}
void guiBindings::clear() { buttonMasks[level]=0; freeList(buttonActions[level]); buttonActions[level]=nullptr; freeList(keyActions[level]); keyActions[level]=nullptr; }
void guiBindings::push() {
    if (level<4) { ++level; buttonMasks[level]=buttonMasks[level-1];
        buttonActions[level]=clone(buttonActions[level-1]); keyActions[level]=clone(keyActions[level-1]); }
}
void guiBindings::pop() { if (level>0) { clear(); --level; } }
void guiBindings::addCode(std::uint32_t code,const char* action,bool button) {
    if (button) { buttonMasks[level]|=code; buttonActions[level]=insert(buttonActions[level],code,action); }
    else keyActions[level]=insert(keyActions[level],code,action);
}
void guiBindings::removeCode(std::uint32_t code,bool button) {
    if (button) { buttonMasks[level]&=~code; buttonActions[level]=removeBindingNode(buttonActions[level],code); }
    else keyActions[level]=removeBindingNode(keyActions[level],code);
}
int guiElement::getWidth() const { return widget?widget->getABox()->max[0]-widget->getABox()->min[0]+1:0; }
int guiElement::getHeight() const { return widget?widget->getABox()->max[1]-widget->getABox()->min[1]+1:0; }
const char* guiElement::getAttr(int which) const {
    if (which<0 || which>5) return nullptr;
    const char* value=attributes[which];
    if (which==3 && widget) {
        // The binary calls puValue_re_eval before returning its inline string
        // buffer. puValue::getStringValue() can instead expose an external
        // valuator pointer without refreshing the cached value.
        char* evaluated=nullptr;
        widget->getValue(&evaluated);
        value=evaluated;
    }
    return value && *value?value:nullptr;
}
void guiElement::setAttr(int which,const char* value) {
    if (which<0 || which>5) return;
    if (which==5) {
        invalidateGuiTooltip(this);
        std::free(attributes[5]); attributes[5]=strings::duplicate(value);
        return;
    }
    std::free(attributes[which]); attributes[which]=strings::duplicate(value);
    if (!widget) return;
    switch (which) {
    case 1:
        widget->setLegend(attributes[1]);
        if (type==4) widget->setSize(width,height);
        if (type==2) { auto font=puGetDefaultLegendFont(); width=puGetStringWidth(font,attributes[1]);
            height=puGetStringHeight(font)+puGetStringDescender(font); widget->setSize(width,height); }
        break;
    case 2: widget->setLabel(attributes[2]); break;
    case 3: widget->setValue(attributes[3]); break;
    default: break;
    }
}
void guiElement::setEH(int which,const char* value) { std::free(eventHandlers[which]); eventHandlers[which]=strings::duplicate(value); }
void guiElement::setPosition(int px,int py) { widget->setPosition(px,py); }
void guiElement::hide() { widget->hide(); }
void guiElement::show() { widget->reveal(); }
void guiElement::release() {
    invalidateGuiTooltip(this);
    if(widget) { widget->setLegend(nullptr); widget->setLabel(nullptr); puDeleteObject(widget); widget=nullptr; }
    for(auto& value:attributes) { std::free(value); value=nullptr; }
    for(auto& value:eventHandlers) { std::free(value); value=nullptr; }
    *this=guiElement{};
}
guiElement* guiSystem::findElement(const char* name) const {
    for (int i=0;i<elementCount;++i) if (elements[i]->attributes[0] && !std::strcmp(name,elements[i]->attributes[0])) return elements[i];
    return nullptr;
}
guiElement* guiElement::initializeFromType(int kind,int px,int py,int w,int h,unsigned option0,unsigned option1,unsigned option2,int option3) {
    *this=guiElement{}; type=kind; x=px; y=py; width=w; height=h;
    const int right=x+width-1,top=y+height-1;
    switch (type) {
    case 0: widget=new GuiRootInterface(x,y); widget->hide(); tabState=-1; break;
    case 1: widget=new GuiFrame(x,y,right,top); tabState=-1; break;
    case 2: widget=new GuiText(x,y); tabState=-1; break;
    case 3: case 15: widget=new GuiSimpleButton(x,y,right,top); widget->setCallback(guiClickCallback); break;
    case 4: case 13: widget=new GuiToggleLozenge(x,y,right,top,option0,option1,option2,option3);
        widget->setCallback(guiClickCallback); break;
    case 14: widget=new GuiLozenge2(x,y,right,top,option0); widget->setCallback(guiLozengeActivateCallback); break;
    case 10: widget=new GuiHeader(x,y,right,top,option0!=0); tabState=-1; break;
    case 6:
        if (height<width) widget=new puSlider(x,y,width,FALSE);
        else widget=new GuiVerticalSlider(x,y,x+16,top,option0,option1);
        widget->setCallback(guiChangeCallback); tabState=-1; break;
    case 7: case 8:
        widget=new GuiBareTextObject(x,y); tabState=-1; break;
    case 11: widget=new GuiShadowBox(x,y,right,top,option0); tabState=-1; break;
    case 12: widget=new GuiStatusBar(x,y,right,top,option0); tabState=-1; break;
    case 16:
        widget=new GuiAnimatedBitmapButton(x,y,right,top);
        if (option0==1) { widget->greyOut(); tabState=-1; }
        if (option0!=2) widget->setCallback(guiClickCallback); else tabState=-1;
        break;
    case 17: widget=new GuiBitmapButton(x,y); tabState=-1; break;
    case 18: widget=new GuiSceneButton; tabState=-1; break;
    case 19: widget=new GuiListButton(option0,option1,option2,x,y,right,top);
        widget->setCallback(guiClickCallback); if (option2&1) tabState=-1; break;
    case 5:
        widget=new puInput(x,y,right,top);
        widget->setCallback(guiChangeCallback);
        break;
    case 9:
        widget=new GuiFilePicker(this,x,y,width,height,2,"base/shapes","Pick a File");
        widget->setCallback(guiSubmitCallback);
        break;
    default: break; // The original leaves unknown type IDs without a widget.
    }
    (void)option3;
    if (widget) widget->setUserData(this);
    return this;
}
guiElement* guiSystem::addElement(int type,int x,int y,int width,int height,
    unsigned option0,unsigned option1,unsigned option2,int option3) {
    // 0040cda0 routes capacity exhaustion through the shared fatal-exit
    // adapter, just like allocation failure. Keep the recovered termination
    // path instead of raising SIGABRT from the host CRT.
    if(elementCount>=256) engine_runtime::engineRuntime_fatalExit_impl();
    auto* element=new guiElement;
    element=element->initializeFromType(type,x,y,width,height,option0,option1,option2,option3);
    elements[elementCount++]=element;
    if(element->tabState>=0) setCurrentTab(element,true);
    refreshAnimationFlag();
    if(type==0) hasInterface=true;
    return element;
}
void guiSystem::removeElement(guiElement* element) {
    int output=0;
    for(int i=0;i<elementCount;++i) if(elements[i]!=element) elements[output++]=elements[i];
    elementCount=output;
    if(element) { element->release(); delete element; }
    if(tabElement==element) tabElement=nullptr;
    if(pressedElement==element) pressedElement=nullptr;
    refreshAnimationFlag();
    if (hasInterface) {
        hasInterface=false;
        for(int i=0;i<elementCount;++i) if(elements[i]->type==0) { hasInterface=true; break; }
    }
}
guiElement* guiSystem::findNearestTab(int dx,int dy,bool playSound) {
    warpCursorOnTab=true;
    auto& runtime=*guiRuntime;
    int cx=tabElement?tabElement->x+tabElement->width/2:runtime.input.mouse.x;
    int cy=tabElement?tabElement->y+tabElement->height/2:runtime.screenHeight-runtime.input.mouse.y;
    if (tabElement && ((tabElement->type==19 && static_cast<GuiListButton*>(tabElement->widget)->handleDirection(dx,dy)) ||
        (tabElement->type==14 && static_cast<GuiLozenge2*>(tabElement->widget)->handleDirection(dx,dy)))) {
        if (playSound) playGuiCancelSound();
        return tabElement;
    }
    int first=0;
    if (hasInterface) for (int i=0;i<elementCount;++i) if (elements[i]->type==0) first=i+1;
    guiElement* best=nullptr; int bestCost=1000000;
    for (int pass=0;pass<3 && !best;++pass) for (int i=first;i<elementCount;++i) {
        auto* candidate=elements[i]; if (candidate==tabElement || candidate->tabState<0) continue;
        const int x=candidate->x+candidate->width/2,y=candidate->y+candidate->height/2;
        int cost=dx?(x-cx)*dx:(y-cy)*dy;
        if (!cost) { if (pass!=2) continue; cost=dx?(y-cy)*dx:(x-cx)*dy; }
        if (cost<0 && pass==1) {
            if (dx>0) cost=runtime.screenWidth-cx+x;
            else if (dy>0) cost=dx?runtime.screenWidth-cx+x:runtime.screenHeight-cy+y;
            else cost=dx?runtime.screenWidth-x+cx:runtime.screenHeight-y+cy;
        }
        if (cost<=0) continue;
        if (dy && (cx<candidate->x || cx>candidate->x+candidate->width)) cost+=(x-cx)*(x-cx);
        else if (dx && (cy<candidate->y || cy>candidate->y+candidate->height)) cost+=(y-cy)*(y-cy);
        if (cost<bestCost) { bestCost=cost; best=candidate; }
    }
    if (best && best!=tabElement && playSound) playGuiCancelSound();
    return best?best:tabElement;
}
guiElement* guiSystem::findMouseOver(int mouseX,int mouseY,int screenHeight) const {
    mouseY=screenHeight-mouseY;
    for(int i=0;i<elementCount;++i) {
        auto* element=elements[i];
        if(element->tabState>=0 && element->x<=mouseX && mouseX<element->x+element->width &&
           element->y<=mouseY && mouseY<element->y+element->height) return element;
    }
    return nullptr;
}
}
