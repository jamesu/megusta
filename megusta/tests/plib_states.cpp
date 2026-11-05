#include "ssg.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <memory>

static unsigned failures=0;
static void check(bool ok,const char* what){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",what);}}
class CurrentStateProbe : public ssgSimpleState {
public:CurrentStateProbe():ssgSimpleState(1){}
};
class TrackedTexture : public ssgTexture {
public:
    static inline unsigned deleted=0;
    ~TrackedTexture()override{++deleted;}
};
static void material(ssgSimpleState& s) {
    s.setColourMaterial(GL_AMBIENT_AND_DIFFUSE);
    s.setMaterial(GL_DIFFUSE,0.2f,0.3f,0.4f,0.5f);
    s.setMaterial(GL_AMBIENT,0.1f,0.1f,0.1f,1);
    s.setMaterial(GL_SPECULAR,1,1,1,1);s.setMaterial(GL_EMISSION,0,0,0,1);
    s.setShininess(12);s.setShadeModel(GL_SMOOTH);s.setAlphaClamp(0.25f);
    s.disable(GL_TEXTURE_2D);s.enable(GL_CULL_FACE);s.enable(GL_COLOR_MATERIAL);
    s.enable(GL_LIGHTING);s.disable(GL_BLEND);s.disable(GL_ALPHA_TEST);
}
int main() {
    CurrentStateProbe cached;
    check(!cached.isEnabled(GL_ALPHA_TEST),"cached alpha test disabled");
    check(cached.isEnabled(GL_CULL_FACE) && cached.isEnabled(GL_COLOR_MATERIAL) && cached.isEnabled(GL_LIGHTING),"cached enabled capabilities");
    check(!cached.isEnabled(GL_BLEND) && !cached.isEnabled(GL_TEXTURE_2D),"cached disabled capabilities");
    ssgSimpleState state;
    check(!state.isTranslucent() && state.getExternalPropertyIndex()==0,"base state defaults");
    for(unsigned i=0;i<256;++i) {
        state.care_about(i);check(state.getCareAbout(i)==0,"care bit clear");
        state.dont_care_about(i);check(uint32_t(state.getCareAbout(i))==(1u<<(i&31u)),"care bit set");
    }
    state.setName("material");state.setExternalPropertyIndex(71);material(state);
    state.setTranslucent();check(state.isTranslucent(),"translucent");state.setOpaque();
    std::unique_ptr<ssgSimpleState> copy(static_cast<ssgSimpleState*>(state.clone(0)));
    check(copy->getExternalPropertyIndex()==71 && !copy->isTranslucent(),"state clone properties");
    check(std::strcmp(copy->getName(),"material")==0 && copy->getRef()==0,"state clone base");
    check(copy->getMaterial(GL_DIFFUSE)[3]==0.5f && copy->getShininess()==12,"state clone material");
    ssgStateSelector selector(2);
    check(selector.getCurrentStep()==&selector && selector.getStep(-1)==&selector && selector.getStep(2)==&selector,"selector fallback identity only");
    auto* first=new ssgSimpleState;auto* second=new ssgSimpleState;material(*first);material(*second);
    selector.setStep(0,first);selector.setStep(1,second);selector.selectStep(0);
    check(selector.getSelectStep()==0 && selector.getCurrentStep()==first,"selected state");
    selector.setTranslucent();check(first->isTranslucent() && selector.isTranslucent(),"selected translucency");selector.setOpaque();
    selector.setMaterial(GL_DIFFUSE,0.7f,0.6f,0.5f,0.4f);
    check(first->getMaterial(GL_DIFFUSE)[3]==0.4f && second->getMaterial(GL_DIFFUSE)[3]==0.5f,"selected material isolation");
    selector.setShininess(33);check(selector.getShininess()==33 && first->getShininess()==33,"selected shininess");
    selector.enable(GL_BLEND);check(first->isEnabled(GL_BLEND) && selector.isEnabled(GL_BLEND),"selected enable");selector.disable(GL_BLEND);
    // Original clone creates an empty destination, then State::copy_from calls
    // its virtual setOpaque/setTranslucent before the step list is installed.
    // The self fallback recurses even with a populated source (0049af10,
    // 00499de0/00499df0). Do not require a successful clone of this original bug.
    check(first->getRef()==1,"selected state ownership");
    {
        auto* texture=new TrackedTexture;texture->ref();texture->setFilename("shared.png");
        {
            ssgSimpleState a,b;a.setTexture(texture);b.setTexture(texture);
            check(texture->getRef()==3 && a.getTexture()==b.getTexture(),"shared texture ownership");
            a.setTexture(static_cast<ssgTexture*>(nullptr));check(texture->getRef()==2,"texture release");
            check(a.getCareAbout(SSG_GL_TEXTURE)!=0 && b.getCareAbout(SSG_GL_TEXTURE)==0,"texture care semantics");
        }
        check(texture->getRef()==1 && TrackedTexture::deleted==0,"external texture owner");
        ssgDeRefDelete(texture);check(TrackedTexture::deleted==1,"last texture owner teardown");
    }
    std::printf("PLIB states/selectors: %u failures\n",failures);return failures?1:0;
}
