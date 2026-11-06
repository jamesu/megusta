#include "pu.h"
#include <cstdio>
#include <cstring>
static unsigned failures=0,callbacks=0,downcallbacks=0;
static puObject* active_during_callback=nullptr;
static void check(bool ok,const char* message){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",message);}}
static void clicked(puObject*){++callbacks;active_during_callback=puActiveWidget();}
static void down(puObject*){++downcallbacks;}
int main(){
    static_assert(sizeof(puFont)==(sizeof(void*)==4 ? 12 : 16),"FNT-only font configuration");
    puSetWindowSize(640,480);
    check(puGetWindowWidth()==640 && puGetWindowHeight()==480,"explicit viewport before window attachment");
    puValue value;int iv=42;value.setValuator(&iv);
    check(value.getValue()==42,"integer binding import");value.setValue(17);check(iv==17,"integer binding export");
    float fv=3.75f;value.setValuator(&fv);check(value.getValue()==3,"float conversion truncation");
    value.setValueRetained(1.0L-2.0L/29.0L);
    check(!std::strcmp(value.getStringValue(),"0.931034") && fv>0.93103f,
          "retained x87 value formats before float storage and updates resource");
    char text[256]="12";value.setValuator(text);value.setValue(7);check(!std::strcmp(text,"7"),"string binding export");
    fntTexFont font; font.setFixedPitch(false);font.setGap(0);
    font.setGlyph('K',0,1,0,1,0,0.5f,0,1,0.5f);
    font.setGlyph('y',0,1,0,1,0,0.5f,-0.25f,0.75f,0.5f);
    puFont metrics(&font,20);
    check(metrics.getStringWidth("KK")==20,"FNT width");
    check(metrics.getStringHeight()==20 && metrics.getStringDescender()==5,"FNT height and descender");
    puSetDefaultFonts(metrics,metrics);
    puButton first(0,0,100,30),second(110,0,210,30);
    first.setLabel("KK");first.setLabelPlace(PUPLACE_LEFT);
    check(first.getBBox()->min[0]==-30,"four-case label bounds");
    first.setLabelPlace(PUPLACE_TOP_CENTER);
    check(first.getBBox()->min[0]==0 && first.getBBox()->max[1]==30,"unsupported placement has no expansion");
    first.setDownCallback(down);second.setCallback(clicked);
    puSetActiveWidget(&first);
    check(!second.checkKey('x',PU_DOWN) && puActiveWidget()==nullptr && downcallbacks==1,"ordinary key deactivates previous widget");
    puSetActiveWidget(&first);second.doHit(PU_LEFT_BUTTON,PU_UP,120,10);
    check(callbacks==1 && active_during_callback==nullptr && puActiveWidget()==&second,"callback precedes active assignment");
    check(second.getValue()==1,"button toggled value");
    second.doHit(PU_LEFT_BUTTON,PU_UP,120,10);
    check(callbacks==2 && active_during_callback==&second && second.getValue()==0,"repeat toggle retains callback state");
    puDeactivateWidget();
    puInput input(0,0,100,30);input.setValue("abcd");input.acceptInput();
    input.setCursor(2);input.setSelectRegion(1,3);
    check(input.checkKey('\b',PU_DOWN) && !std::strcmp(input.getStringValue(),"acd") && input.getCursor()==1,
          "input backspace removes one character despite selection");
    check(!input.checkKey(0x03,PU_DOWN),"input rejects unsupported clipboard shortcut");
    input.setValue("abcd");input.setSelectRegion(1,3);input.setCursor(2);
    check(input.checkKey(0x7f,PU_DOWN) && !std::strcmp(input.getStringValue(),"ad") && input.getCursor()==2,
          "input delete removes selection without resetting cursor");
    char item0[]="K",item1[]="y";char* items[]={item0,item1,nullptr};
    puListBox list(0,0,100,100,items);list.setCallback(clicked);
    list.doHit(PU_LEFT_BUTTON,PU_DOWN,5,90);
    check(callbacks==3 && list.getValue()==0 && active_during_callback==nullptr && puActiveWidget()==&list,
          "list selects on down and invokes callback before active assignment");
    list.doHit(PU_LEFT_BUTTON,PU_UP,5,65);
    check(callbacks==4 && list.getValue()==1 && active_during_callback==&list,
          "list also selects on up independently of configured edge");
    puDeactivateWidget();
    puSlider slider(0,0,100,0,10); slider.setCallback(clicked);
    slider.doHit(PU_LEFT_BUTTON,PU_DOWN,50,0);
    float slider_value=0; slider.getValue(&slider_value);
    check(slider_value==0.5f && callbacks==5 && active_during_callback==nullptr && puActiveWidget()==&slider,
          "slider midpoint and callback before activation");
    slider.doHit(PU_LEFT_BUTTON,PU_DOWN,-10,0); slider.getValue(&slider_value);
    check(slider_value==0.0f && callbacks==6,"slider clamps below range");
    slider.doHit(PU_LEFT_BUTTON,PU_DOWN,110,0); slider.getValue(&slider_value);
    check(slider_value==1.0f && callbacks==7,"slider clamps above range");
    slider.setCBMode(PUSLIDER_DELTA); slider.setDelta(0.2f);
    slider.doHit(PU_LEFT_BUTTON,PU_DOWN,90,0);
    check(callbacks==7,"slider delta suppresses small changes");
    slider.doHit(PU_LEFT_BUTTON,PU_DOWN,50,0);
    check(callbacks==8,"slider delta invokes for large changes");
    puDeactivateWidget();
    puSlider vertical(0,0,100,1,10); vertical.setCallback(clicked);
    vertical.doHit(PU_LEFT_BUTTON,PU_DOWN,0,50); vertical.getValue(&slider_value);
    check(slider_value==0.5f && callbacks==9 && puActiveWidget()==&vertical,
          "vertical slider uses y coordinate");
    puDeactivateWidget();
    {
        puInterface parent(0,0);
        puFilePicker* picker=new puFilePicker(0,0,220,170,1,".","Files");
        picker->setSize(190,160);
        check((picker->getType() & PUCLASS_FILEPICKER)!=0,"file picker constructs and resizes");
        delete picker;
        parent.close();
    }
    std::printf("PLIB PUI/FNT logic: %u failures; graphics parity pending\n",failures);
    return failures?1:0;
}
