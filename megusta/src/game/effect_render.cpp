#include "effect.h"
#include "engine/multi_texture.h"
#include "../engine/opengl_adapter.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glpng.h>
#include <cmath>
namespace megusta {
extern EffectRuntime* effectRuntime;
static void configureTexture(Effect& effect) {
    auto& gl=effectRuntime->gl;
    gl.enable(0x4a); gl.bindTexture(0x4a,effect.textureId);
    glTexParameteri(gl.tokens[0x4a],gl.tokens[0x4d],gl.tokens[0x50]);
    glTexParameteri(gl.tokens[0x4a],gl.tokens[0x4e],gl.tokens[0x50]);
    gl.matrixMode(0xe);
}
static void rampParticle(const Effect& effect,const EffectParticle& particle,float& alpha,float& extent) {
    if (effect.rampEnabled) {
        alpha=static_cast<float>(particle.remainingTicks)/particle.initialTicks;
        extent=effect.scale[0]==0 ? 1 : (2-alpha)*effect.scale[0];
    }
}
static void drawQuad(float left,float right,float bottom,float top,float z,bool flip) {
    auto& gl=effectRuntime->gl;
    gl.begin(0xb);
    gl.texCoord2f(0,flip ? 1 : 0); gl.vertex3f(left,bottom,z);
    gl.texCoord2f(1,flip ? 1 : 0); gl.vertex3f(right,bottom,z);
    gl.texCoord2f(1,flip ? 0 : 1); gl.vertex3f(right,top,z);
    gl.texCoord2f(0,flip ? 0 : 1); gl.vertex3f(left,top,z);
    glEnd();
}
static void lensPass(Effect& effect) {
    auto& runtime=*effectRuntime;
    auto& gl=runtime.gl;
    auto load=[&](std::uint32_t& texture,const char* path) {
        if (!texture) {
            pngInfo info;
            texture=pix::bindPngTexture(path,0,gl.tokens[0x58],gl.tokens[0x4f],gl.tokens[0x4f]);
        }
    };
    load(runtime.sharedTexture1,"data/textures/effects/lens1.png");
    load(runtime.sharedTexture0,"data/textures/effects/lens2.png");
    load(runtime.sharedTexture3,"data/textures/effects/lens3.png");
    load(runtime.lensTexture,"data/textures/effects/lens4.png");
    const float* m=runtime.cameraMatrix;
    float x=m[0]*effect.position[0]+m[4]*effect.position[1]+m[8]*effect.position[2]+m[12];
    float y=m[1]*effect.position[0]+m[5]*effect.position[1]+m[9]*effect.position[2]+m[13];
    float z=m[2]*effect.position[0]+m[6]*effect.position[1]+m[10]*effect.position[2]+m[14];
    long double length=std::sqrt(static_cast<long double>(x*x+y*y+z*z));
    if (!((1.0L/length)*z<=-0.8L)) return;
    x-=20;
    float radius=static_cast<float>(std::sqrt(static_cast<long double>(x*x+y*y)));
    x/=radius; y/=radius;
    gl.color4f(1,1,1,1); gl.disable(0x21); gl.depthMask(false);
    gl.enable(0x4a); gl.enable(0x2e); gl.blendFunc(0x32,0x32);
    glTexParameteri(gl.tokens[0x4a],gl.tokens[0x4d],gl.tokens[0x50]);
    glTexParameteri(gl.tokens[0x4a],gl.tokens[0x4e],gl.tokens[0x50]);
    gl.matrixMode(0xe); glLoadIdentity();
    for (int i=0;i<7;++i) {
        auto& entry=effectLensRecords[i];
        float centerX=x*radius*entry.positionFactor,centerY=y*radius*entry.positionFactor;
        entry.intensity=1;
        // The saved body binds texture1 for every table row.
        gl.bindTexture(0x4a,runtime.sharedTexture1);
        float extent=entry.sizeFactor*50;
        drawQuad(centerX-extent,centerX+extent,centerY+extent,centerY-extent,-200,true);
    }
    gl.enable(0x21); gl.depthMask(true);
}
void Effect::drawWorldStyles() {
    if (!emissionState) return;
    if (style==5) { drawOrientedParticles(); return; }
    if (style==6) { lensPass(*this); return; }
    if (style==9) { drawLocalParticles(); return; }
    if (style!=1 && style!=2 && style!=4 && style!=7) return;
    auto& gl=effectRuntime->gl;
    const float* m=effectRuntime->cameraMatrix;
    gl.enable(0x2e);
    // Style7 sets blend before depth mask in the saved body.
    if (style==7) { gl.blendFunc(0x37,0x32); gl.enable(0x4a); gl.depthMask(false); }
    else {
        gl.depthMask(false);
        gl.blendFunc(style==2 && blendMode!=0 ? 0x31 : 0x37,
                     style==2 && blendMode!=0 ? 0x38 : 0x32);
    }
    if (style==1) { gl.enable(0x4a); gl.disable(0x18); }
    configureTexture(*this);
    if (style!=1) glLoadIdentity();
    float alpha=1,extent=scale[0]==0 ? 1 : scale[0];
    for (auto& particle:particles) {
        if (!particle.remainingTicks) continue;
        float x=particle.transform[12],y=particle.transform[13],z=particle.transform[14];
        if (style!=7 && ground[2]!=0 && z<ground[2]) { particle.remainingTicks=0; continue; }
        if (style==1) {
            gl.loadMatrix(m); gl.multiplyMatrix(particle.transform);
            rampParticle(*this,particle,alpha,extent);
            gl.color4f(particle.colorRGB[0],particle.colorRGB[1],particle.colorRGB[2],alpha);
            drawQuad(-extent,extent,extent,-extent,0,true);
        } else if (style==7) {
            // Preserve the saved body's unusual fixed Z=1 and Z=2 endpoints.
            float headX=m[0]*x+m[4]*y+m[12]+m[8];
            float headY=m[1]*x+m[5]*y+m[13]+m[9];
            float headZ=m[2]*x+m[6]*y+m[14]+m[10];
            float sx=particle.spawnPosition[0],sy=particle.spawnPosition[1];
            float tailX=m[8]+m[8]+m[0]*sx+m[4]*sy+m[12];
            float tailY=m[9]+m[9]+m[1]*sx+m[5]*sy+m[13];
            float tailZ=m[10]+m[10]+m[2]*sx+m[6]*sy+m[14];
            rampParticle(*this,particle,alpha,extent);
            gl.color4f(particle.colorRGB[0],particle.colorRGB[1],particle.colorRGB[2],alpha);
            gl.begin(0xb);
            gl.texCoord2f(0,1); gl.vertex3f(headX-extent,headY+extent,headZ);
            gl.texCoord2f(1,1); gl.vertex3f(headX+extent,headY+extent,headZ);
            gl.texCoord2f(1,0); gl.vertex3f(tailX+extent,tailY-extent,tailZ);
            gl.texCoord2f(0,0); gl.vertex3f(tailX-extent,tailY-extent,tailZ);
            glEnd();
        } else {
            float viewX=m[0]*x+m[4]*y+m[8]*z+m[12];
            float viewY=m[1]*x+m[5]*y+m[9]*z+m[13];
            float viewZ=m[2]*x+m[6]*y+m[10]*z+m[14];
            rampParticle(*this,particle,alpha,extent);
            gl.color4f(particle.colorRGB[0],particle.colorRGB[1],particle.colorRGB[2],alpha);
            drawQuad(viewX-extent,viewX+extent,viewY-extent,viewY+extent,viewZ,style==4);
        }
    }
    gl.depthMask(true);
}
void Effect::drawOrientedParticles() {
    auto& gl=effectRuntime->gl;
    gl.disable(0x18); gl.enable(0x2e); gl.depthMask(false); gl.blendFunc(0x37,0x32);
    configureTexture(*this);
    float extent=scale[0]==0 ? 1 : scale[0];
    for (const auto& particle:particles) {
        if (!particle.remainingTicks) continue;
        gl.loadMatrix(effectRuntime->cameraMatrix); gl.multiplyMatrix(particle.transform);
        gl.color4f(particle.colorRGB[0],particle.colorRGB[1],particle.colorRGB[2],1);
        gl.begin(0xb);
        gl.texCoord2f(0,1); gl.vertex3f(-extent,extent,0);
        gl.texCoord2f(1,1); gl.vertex3f(extent,extent,0);
        gl.texCoord2f(1,0); gl.vertex3f(extent,-extent,0);
        gl.texCoord2f(0,0); gl.vertex3f(-extent,-extent,0);
        glEnd();
    }
    gl.depthMask(true);
}
void Effect::drawLocalParticles() {
    auto& gl=effectRuntime->gl;
    gl.enable(0x2e); gl.depthMask(false); gl.blendFunc(0x31,0x38); gl.enable(0x4a); gl.enable(0x18);
    configureTexture(*this);
    float alpha=1,extent=scale[0]==0 ? 1 : scale[0];
    gl.loadMatrix(effectRuntime->cameraMatrix);
    gl.translate(position[0],position[1],position[2]); gl.rotate(angleDegrees,0,0,1);
    for (auto& particle:particles) {
        if (!particle.remainingTicks) continue;
        float x=particle.transform[12],y=particle.transform[13],z=particle.transform[14];
        if (!(ground[2]==0 || ground[2]<=z)) { particle.remainingTicks=0; continue; }
        if (rampEnabled) {
            alpha=static_cast<float>(particle.remainingTicks)/particle.initialTicks;
            extent=scale[0]==0 ? 1 : (2-alpha)*scale[0];
        }
        gl.color4f(particle.colorRGB[0],particle.colorRGB[1],particle.colorRGB[2],alpha);
        gl.begin(0xb);
        gl.texCoord2f(0,1); gl.vertex3f(x-extent,y-extent,z);
        gl.texCoord2f(1,1); gl.vertex3f(x+extent,y-extent,z);
        gl.texCoord2f(1,0); gl.vertex3f(x+extent,y+extent,z);
        gl.texCoord2f(0,0); gl.vertex3f(x-extent,y+extent,z);
        glEnd();
    }
    gl.depthMask(true);
}
void Effect::drawOverlayStyles() {
    if (emissionState!=0 && (style==3 || style==8)) drawScreenParticles();
}
void Effect::drawScreenParticles() {
    auto& gl=effectRuntime->gl;
    gl.enable(0x2e); gl.depthMask(false); gl.blendFunc(0x37,0x32);
    gl.enable(0x4a); gl.bindTexture(0x4a,textureId);
    glTexParameteri(gl.tokens[0x4a],gl.tokens[0x4d],gl.tokens[0x50]);
    glTexParameteri(gl.tokens[0x4a],gl.tokens[0x4e],gl.tokens[0x50]);
    gl.matrixMode(0xe); glLoadIdentity();
    float alpha=1, extent=scale[0]==0 ? 1 : scale[0];
    for (const auto& particle:particles) {
        if (!particle.remainingTicks) continue;
        float x=particle.transform[12],y=particle.transform[13];
        if (rampEnabled) {
            alpha=static_cast<float>(particle.remainingTicks)/particle.initialTicks;
            extent=scale[0]==0 ? 1 : (2-alpha)*scale[0];
        }
        gl.color4f(particle.colorRGB[0],particle.colorRGB[1],particle.colorRGB[2],alpha);
        gl.begin(0xb);
        gl.texCoord2f(0,0); gl.vertex2f(x-extent,y-extent);
        gl.texCoord2f(1,0); gl.vertex2f(x+extent,y-extent);
        gl.texCoord2f(1,1); gl.vertex2f(x+extent,y+extent);
        gl.texCoord2f(0,1); gl.vertex2f(x-extent,y+extent);
        glEnd();
    }
    gl.depthMask(true);
}
}
