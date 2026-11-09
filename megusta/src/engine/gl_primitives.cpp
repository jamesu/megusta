#include "opengl_adapter.h"
#include <GL/gl.h>
#include <cmath>
namespace megusta {
namespace {
long double sine(float angle) { return std::sin(static_cast<long double>(angle)); }
long double cosine(float angle) { return std::cos(static_cast<long double>(angle)); }
}
void OpenGLAdapter::emitNormalizedNormal(float x,float y,float z) {
    float square=z*z+y*y+x*x; long double length=std::sqrt(static_cast<long double>(square));
    if (length>static_cast<long double>(1e-5)) {
        x=static_cast<float>(x/length); y=static_cast<float>(y/length); z=static_cast<float>(z/length);
    }
    normal3f(x,y,z);
}
void OpenGLAdapter::drawTorus(float inner,int sides,float outer,int rings) {
    for (int side=0;side<sides;++side) {
        begin(0xc);
        for (int ring=0;ring<=rings;++ring) {
            float around=(static_cast<float>(ring%rings)*6.2831855)/static_cast<float>(rings);
            for (int pair=1;pair>=0;--pair) {
                float section=((static_cast<float>((side+pair)%sides)+0.5)*6.2831855)/static_cast<float>(sides);
                long double ca=cosine(around),sa=sine(around),cs=cosine(section),ss=sine(section);
                normal3f(static_cast<float>(cs*static_cast<float>(ca)),static_cast<float>(cs*static_cast<float>(sa)),static_cast<float>(ss));
                float radial=static_cast<float>(cs*inner+outer);
                vertex3f(static_cast<float>(ca*radial),static_cast<float>(sa*radial),static_cast<float>(ss*inner));
            }
        }
        glEnd();
    }
}
void OpenGLAdapter::drawCylinder(float base,float top,float height,int slices,int stacks) {
    float stepS=1.0/static_cast<float>(slices),angleStep=6.2831855/static_cast<float>(slices);
    float radiusDelta=top-base,heightStep=height/static_cast<float>(stacks),slope=(base-top)/height;
    float s=0;
    for (int slice=0;slice<slices;++slice) {
        float angle=static_cast<float>(slice)*angleStep;
        long double s0=sine(angle),c0=cosine(angle);
        angle=static_cast<float>(slice+1)*angleStep;
        long double s1=sine(angle),c1=cosine(angle);
        float z=0,radius=base,t=0; begin(0xc);
        for (int stack=0;stack<=stacks;++stack) {
            float x0=static_cast<float>(-s0),y0=static_cast<float>(c0);
            float x1=static_cast<float>(-s1),y1=static_cast<float>(c1);
            emitNormalizedNormal(x0,y0,slope); texCoord2f(s,t); vertex3f(radius*x0,radius*y0,z);
            emitNormalizedNormal(x1,y1,slope); texCoord2f(s+stepS,t); vertex3f(radius*x1,radius*y1,z);
            z+=heightStep; radius+=radiusDelta/static_cast<float>(stacks); t+=1.0/static_cast<float>(stacks);
        }
        glEnd(); s+=stepS;
    }
}
void OpenGLAdapter::drawSphere(float radius,int slices,int stacks) {
    float latitudeStep=3.1415927/static_cast<float>(stacks),longitudeStep=6.2831855/static_cast<float>(slices);
    auto point=[&](float latitude,float longitude,float* output) {
        float slat=static_cast<float>(sine(latitude));
        output[0]=static_cast<float>(-sine(longitude)*slat);
        output[1]=static_cast<float>(cosine(longitude)*slat); output[2]=static_cast<float>(cosine(latitude));
    };
    auto normalAndVertex=[&](const float* n) {
        normal3f(n[0],n[1],n[2]); vertex3f(n[0]*radius,n[1]*radius,n[2]*radius);
    };
    begin(10); normal3f(0,0,1); texCoord2f(0.5f,0); vertex3f(0,0,radius);
    for (int slice=0;slice<=slices;++slice) {
        float longitude=slice==slices ? 0.0f : static_cast<float>(slice)*longitudeStep;
        float n[3]; point(latitudeStep,longitude,n); normalAndVertex(n);
    }
    glEnd();
    float vertical=1.0f,stepT=1.0/static_cast<float>(stacks);
    for (int stack=0;stack<stacks;++stack) {
        float latitude=static_cast<float>(stack)*latitudeStep,next=latitude+latitudeStep;
        float horizontal=0; begin(0xc);
        for (int slice=0;slice<=slices;++slice) {
            float longitude=slice==slices ? 0.0f : static_cast<float>(slice)*longitudeStep;
            float n[3]; point(latitude,longitude,n); normal3f(n[0],n[1],n[2]); texCoord2f(horizontal,1.0-vertical);
            vertex3f(n[0]*radius,n[1]*radius,n[2]*radius);
            point(next,longitude,n); normal3f(n[0],n[1],n[2]); texCoord2f(horizontal,1.0-(vertical-stepT));
            horizontal+=1.0/static_cast<float>(slices); vertex3f(n[0]*radius,n[1]*radius,n[2]*radius);
        }
        glEnd(); vertical-=stepT;
    }
    begin(10); normal3f(0,0,-1); texCoord2f(0.5f,1); vertex3f(0,0,-radius);
    float latitude=3.1415927-latitudeStep,horizontal=1.0f;
    for (int slice=slices;slice>=0;--slice) {
        float longitude=slice==slices ? 0.0f : static_cast<float>(slice)*longitudeStep;
        float n[3]; point(latitude,longitude,n); normal3f(n[0],n[1],n[2]); texCoord2f(horizontal,1.0-stepT);
        horizontal-=1.0/static_cast<float>(slices); vertex3f(n[0]*radius,n[1]*radius,n[2]*radius);
    }
    glEnd();
}
}
