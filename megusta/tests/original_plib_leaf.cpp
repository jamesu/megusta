// Narrow original-code oracle DLL, loaded into a suspended original process.
// Invoke bounded SG/state/NET/PUI functions and the SSG serializer with redirected
// file/allocation services; never resume the game entry point. Hashes are pinned.
// Raw byte layouts exist solely here at the original binary boundary.
#include "ssgLocal.h"
#include "pix_texture.h"
#include "megusta_ase_support.h"
#include "ssgParser.h"
#include "ssgASX.h"
#include "ssgGameAnimation.h"
#include <GL/glpng.h>
extern "C" unsigned char megusta_glpng_gammaByte(unsigned char);
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <set>
#include <memory>
#include <cstdlib>
#ifdef MEGUSTA_ORACLE_PUI
#include "pu.h"
#endif
#ifdef MEGUSTA_ORACLE_NET
#include "netRoom.h"
static void net_error_checks();
#endif

static unsigned checks=0;
static unsigned mismatches=0;
static std::set<uint32_t> compared_functions;
static void compare(bool ok,uint32_t address,const char* label) {
    ++checks;
    compared_functions.insert(address);
    if(!ok){++mismatches;std::fprintf(stderr,"Mismatch %08x: %s\n",address,label);}
}
struct OriginalImage {
    OriginalImage() {
        auto* module=reinterpret_cast<unsigned char*>(GetModuleHandleA(nullptr));
        if(module!=reinterpret_cast<unsigned char*>(0x400000))throw std::runtime_error("Oracle must run inside the suspended original process");
        auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
        auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS32*>(module+dos->e_lfanew);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE || pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386)throw std::runtime_error("Wrong original process image");
    }
};
struct BinaryObject {
    alignas(4) std::array<unsigned char,0x90> bytes{};
    void set(unsigned offset,uint32_t value){std::memcpy(bytes.data()+offset,&value,4);}
    uint32_t get(unsigned offset)const{uint32_t v;std::memcpy(&v,bytes.data()+offset,4);return v;}
    void* pointer(){return bytes.data();}
};
template<class F>static F at(uint32_t address){compared_functions.insert(address);return reinterpret_cast<F>(uintptr_t(address));}
using Set=void(__attribute__((thiscall)) *)(void*,uint32_t);
using Get=uint32_t(__attribute__((thiscall)) *)(void*);
using Query=uint32_t(__attribute__((thiscall)) *)(void*,uint32_t);
using Action=void(__attribute__((thiscall)) *)(void*);
using PtrGet=void*(__attribute__((thiscall)) *)(void*);
using PtrQuery=void*(__attribute__((thiscall)) *)(void*,int);

static void state_checks() {
    BinaryObject binary;binary.set(0x24,0xffffffffu);
    ssgSimpleState native;
    for(unsigned mode=0;mode<256;++mode) {
        at<Set>(0x46d140)(binary.pointer(),mode);native.care_about(mode);
        compare(at<Query>(0x46d180)(binary.pointer(),mode)==uint32_t(native.getCareAbout(mode)),0x46d180,"care query after clear");
        compare(binary.get(0x24)==(0xffffffffu & ~(1u<<(mode&31u))),0x46d140,"x86 count masking");
        at<Set>(0x46d160)(binary.pointer(),mode);native.dont_care_about(mode);
        compare(at<Query>(0x46d180)(binary.pointer(),mode)==uint32_t(native.getCareAbout(mode)),0x46d160,"care query after set");
        compare(binary.get(0x24)==0xffffffffu,0x46d160,"restored mask");
    }
}
static void retained_dot_checks() {
    using Dot=long double(__cdecl *)(const float*,const float*);
    const sgVec3 a[]={{1,2,3},{1e20f,1,-1e20f},{0.000001f,-0.3f,1.7f},{-2,4,9}};
    const sgVec3 b[]={{4,5,6},{1,1,1},{-1e10f,0.7f,0.25f},{7,-3,0.125f}};
    for(unsigned i=0;i<4;++i)
        compare(at<Dot>(0x463e80)(a[i],b[i])==sgScalarProductVec3Retained(a[i],b[i]),0x463e80,"retained Z-Y-X dot product");
}
static void gamma_checks() {
    using Build=void(__cdecl *)(double);
    for(double gamma:{0.0,1.0,0.5,1.2,1.45,2.2}) {
        at<Build>(0x461990)(gamma);pngSetViewingGamma(gamma);
        auto *original=reinterpret_cast<unsigned char *>(0x53de5c);
        for(unsigned i=0;i<256;++i)
            compare(original[i]==megusta_glpng_gammaByte(static_cast<unsigned char>(i)),0x461990,"TXF gamma lookup");
    }
    pngSetViewingGamma(1.0);
}
#ifdef MEGUSTA_ORACLE_NET
static void net_error_checks() {
    using Error=const char*(__cdecl *)(int);
    for(int code:{0,61,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,200})
        compare(!std::strcmp(at<Error>(0x46e750)(code),netErrorString(code)),0x46e750,"network error text");
}
#endif

static void capability_checks() {
    BinaryObject b;b.set(0,0x4ebbb4);b.set(0x24,0xffffffffu);b.set(0x28,0);
    ssgSimpleState native;
    const uint32_t oldmode[]={0x4a,0x18,0x76,0x67,0x2e,0x23};
    const GLenum mode[]={GL_TEXTURE_2D,GL_CULL_FACE,GL_COLOR_MATERIAL,GL_LIGHTING,GL_BLEND,GL_ALPHA_TEST};
    for(unsigned i=0;i<6;++i) {
        at<Set>(0x46cac0)(b.pointer(),oldmode[i]);native.enable(mode[i]);
        compare(at<Query>(0x46c8b0)(b.pointer(),oldmode[i])==uint32_t(native.isEnabled(mode[i])),0x46cac0,"enable mask");
        compare(at<Query>(0x46d180)(b.pointer(),i)==uint32_t(native.getCareAbout(i)),0x46d180,"virtual care dispatch");
        at<Set>(0x46c990)(b.pointer(),oldmode[i]);native.disable(mode[i]);
        compare(at<Query>(0x46c8b0)(b.pointer(),oldmode[i])==uint32_t(native.isEnabled(mode[i])),0x46c990,"disable mask");
    }
    compare(at<Query>(0x46c8b0)(b.pointer(),0)==uint32_t(native.isEnabled(0)),0x46c8b0,"unknown capability query");
    at<Action>(0x46d120)(b.pointer());native.setTranslucent();
    compare(at<Get>(0x46d110)(b.pointer())==uint32_t(native.isTranslucent()),0x46d120,"translucent");
    at<Action>(0x46d130)(b.pointer());native.setOpaque();
    compare(at<Get>(0x46d110)(b.pointer())==uint32_t(native.isTranslucent()),0x46d130,"opaque");
}
static int identity(void* p,void* self,void* a,void* b) {
    if(p==self)return 0;if(p==a)return 1;if(p==b)return 2;
    throw std::runtime_error("Unexpected state pointer");
}
static void selector_checks() {
    BinaryObject binary,raw_a,raw_b;
    std::array<void*,2> entries={raw_a.pointer(),raw_b.pointer()};
    binary.set(0x84,2);binary.set(0x88,0xffffffffu);binary.set(0x8c,reinterpret_cast<uintptr_t>(entries.data()));
    ssgStateSelector native(2);auto* a=new ssgSimpleState;auto* b=new ssgSimpleState;
    native.setStep(0,a);native.setStep(1,b);
    for(uint32_t selection:{0u,1u,2u,0xffffffffu,0x80000000u}) {
        at<Set>(0x499cf0)(binary.pointer(),selection);native.selectStep(selection);
        compare(at<Get>(0x499d00)(binary.pointer())==native.getSelectStep(),0x499cf0,"selected index");
        compare(identity(at<PtrGet>(0x499d10)(binary.pointer()),binary.pointer(),raw_a.pointer(),raw_b.pointer())==identity(native.getCurrentStep(),&native,a,b),0x499d10,"current step identity");
        for(int i:{-1,0,1,2})
            compare(identity(at<PtrQuery>(0x499d40)(binary.pointer(),i),binary.pointer(),raw_a.pointer(),raw_b.pointer())==identity(native.getStep(i),&native,a,b),0x499d40,"step identity");
    }
    entries[1]=nullptr;native.setStep(1,nullptr);native.selectStep(1);at<Set>(0x499cf0)(binary.pointer(),1);
    compare(at<PtrGet>(0x499d10)(binary.pointer())==binary.pointer() && native.getCurrentStep()==&native,0x499d10,"empty slot fallback");
}

static uint32_t random_word() {static uint32_t state=0x5a17e2d1;state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static float finite_float() {
    uint32_t bits=random_word();bits=(bits&0x807fffffu)|(((bits>>24)%200u+20u)<<23);
    float result;std::memcpy(&result,&bits,4);return result;
}
static void math_checks() {
    using Cross=void(__cdecl *)(sgVec3,const sgVec3,const sgVec3);
    using Multiply=void(__cdecl *)(sgMat4,const sgMat4,const sgMat4);
    for(unsigned i=0;i<256;++i) {
        sgVec3 a,b,oldcross,newcross;
        for(unsigned j=0;j<3;++j){a[j]=finite_float();b[j]=finite_float();}
        at<Cross>(0x456b10)(oldcross,a,b);sgVectorProductVec3(newcross,a,b);
        compare(std::memcmp(oldcross,newcross,sizeof(oldcross))==0,0x456b10,"bitwise vector product");
        sgMat4 left,right,oldproduct,newproduct;
        for(int c=0;c<4;++c)for(int r=0;r<4;++r){left[c][r]=finite_float();right[c][r]=finite_float();}
        at<Multiply>(0x457cc0)(oldproduct,left,right);sgMultMat4(newproduct,left,right);
        compare(std::memcmp(oldproduct,newproduct,sizeof(oldproduct))==0,0x457cc0,"bitwise matrix product");
    }
}
static void geometry_checks() {
    using Two=void(__cdecl *)(sgMat4,const sgMat4);
    using Rot=void(__cdecl *)(sgMat4,float,const sgVec3);
    using Look=void(__cdecl *)(sgMat4,const sgVec3,const sgVec3,const sgVec3);
    using Coord=void(__cdecl *)(sgMat4,float,float,float,float,float,float);
    using Trans=void(__cdecl *)(sgMat4,const sgVec3);
    using TransXYZ=void(__cdecl *)(sgMat4,float,float,float);
    using Normal=void(__cdecl *)(sgVec3,const sgVec3,const sgVec3,const sgVec3);
    using Extend=void(__attribute__((thiscall)) *)(void*,const void*);
    for(unsigned i=0;i<32;++i) {
        sgVec3 point={float(float(i)*0.5f),float(-float(i)),float(float(i)+1)},axis={0,0,1},eye={point[0],point[1],float(point[2]+2)},up={0,1,0};
        sgMat4 old{},now{};
        at<Trans>(0x457a50)(old,point);sgMakeTransMat4(now,point);compare(!std::memcmp(old,now,sizeof(old)),0x457a50,"vector translation");
        at<TransXYZ>(0x457aa0)(old,point[0],point[1],point[2]);sgMakeTransMat4(now,point[0],point[1],point[2]);compare(!std::memcmp(old,now,sizeof(old)),0x457aa0,"xyz translation");
        at<Rot>(0x456b50)(old,float(i)*5,axis);sgMakeRotMat4(now,float(i)*5,axis);compare(!std::memcmp(old,now,sizeof(old)),0x456b50,"axis rotation");
        at<Look>(0x456ca0)(old,eye,point,up);sgMakeLookAtMat4(now,eye,point,up);compare(!std::memcmp(old,now,sizeof(old)),0x456ca0,"look-at matrix");
        at<Coord>(0x457890)(old,point[0],point[1],point[2],float(i),float(i)*2,float(i)*3);sgMakeCoordMat4(now,point[0],point[1],point[2],float(i),float(i)*2,float(i)*3);compare(!std::memcmp(old,now,sizeof(old)),0x457890,"coordinate matrix");
        sgMat4 left,right;sgMakeTransMat4(left,point);sgMakeRotMat4(right,20,axis);
        std::memcpy(old,left,sizeof(old));std::memcpy(now,left,sizeof(now));at<Two>(0x457ba0)(old,right);sgPreMultMat4(now,right);compare(!std::memcmp(old,now,sizeof(old)),0x457ba0,"pre multiplication");
        std::memcpy(old,left,sizeof(old));std::memcpy(now,left,sizeof(now));at<Two>(0x457c30)(old,right);sgPostMultMat4(now,right);compare(!std::memcmp(old,now,sizeof(old)),0x457c30,"post multiplication");
        at<Two>(0x457d70)(old,left);sgTransposeNegateMat4(now,left);compare(!std::memcmp(old,now,sizeof(old)),0x457d70,"transpose negate");
        sgVec3 oldnormal,nownormal;at<Normal>(0x457af0)(oldnormal,point,eye,up);sgMakeNormal(nownormal,point,eye,up);compare(!std::memcmp(oldnormal,nownormal,sizeof(oldnormal)),0x457af0,"triangle normal");
        std::array<float,6> rawbox={1,1,1,-1,-1,-1};sgBox box;box.empty();
        at<Extend>(0x456e60)(rawbox.data(),point);box.extend(point);
        compare(!std::memcmp(rawbox.data(),box.getMin(),12) && !std::memcmp(rawbox.data()+3,box.getMax(),12),0x456e60,"empty box extends to point");
        std::array<float,6> other={-2,-3,-4,2,3,4};sgBox otherbox;sgVec3 lo={-2,-3,-4},hi={2,3,4};otherbox.empty();otherbox.extend(lo);otherbox.extend(hi);
        at<Extend>(0x456f30)(rawbox.data(),other.data());box.extend(&otherbox);
        compare(!std::memcmp(rawbox.data(),box.getMin(),12) && !std::memcmp(rawbox.data()+3,box.getMax(),12),0x456f30,"box union");
        std::array<float,4> rawball={0,0,0,2};sgSphere ball;sgVec3 center={0,0,0};ball.setCenter(center);ball.setRadius(2);
        at<Extend>(0x456fd0)(rawbox.data(),rawball.data());box.extend(&ball);
        compare(!std::memcmp(rawbox.data(),box.getMin(),12) && !std::memcmp(rawbox.data()+3,box.getMax(),12),0x456fd0,"box extends by sphere");
        rawball={0,0,0,-1};ball.empty();at<Extend>(0x457050)(rawball.data(),point);ball.extend(point);
        compare(!std::memcmp(rawball.data(),ball.getCenter(),12) && rawball[3]==ball.getRadius(),0x457050,"empty sphere point");
        at<Extend>(0x457110)(rawball.data(),other.data());ball.extend(&otherbox);
        compare(!std::memcmp(rawball.data(),ball.getCenter(),12) && rawball[3]==ball.getRadius(),0x457110,"sphere encloses box");
        std::array<float,4> second={3,4,5,2};sgSphere otherball;sgVec3 secondcenter={3,4,5};otherball.setCenter(secondcenter);otherball.setRadius(2);
        at<Extend>(0x4572c0)(rawball.data(),second.data());ball.extend(&otherball);
        compare(!std::memcmp(rawball.data(),ball.getCenter(),12) && rawball[3]==ball.getRadius(),0x4572c0,"sphere union");
    }
}
static void frustum_checks() {
    using Contains=int(__attribute__((thiscall)) *)(void*,const void*);
    for(unsigned i=0;i<16;++i) {
        std::array<float,36> original{};
        original[0]=-2.0f-float(i)*0.25f;original[1]=2.0f;original[2]=3.0f;original[3]=-3.0f;original[4]=1;original[5]=100;
        at<Action>(0x4573b0)(original.data());
        sgFrustum native;native.setFrustum(original[0],original[1],original[3],original[2],1,100);
        sgMat4 matrix;native.getMat4(matrix);
        compare(!std::memcmp(original.data()+18,matrix,sizeof(matrix)),0x4573b0,"explicit frustum projection matrix");
        for(const std::array<float,4>& sphere:std::vector<std::array<float,4>>{{0,0,-3,0.5f},{0,0,3,0.5f},{100,0,-3,1},{0,0,-100,1},{0,0,-1,1},{0,100,-3,1}}) {
            sgSphere ball;sgVec3 center={sphere[0],sphere[1],sphere[2]};ball.setCenter(center);ball.setRadius(sphere[3]);
            compare(at<Contains>(0x457730)(original.data(),sphere.data())==native.contains(&ball),0x457730,"frustum sphere inside/outside/straddle");
        }
    }
    // MG-PLIB-027 changed the FOV backend/rounding, so compare that adapted
    // path specifically. Clear unchanged setters do not need new test cases.
    for(float near_distance:{0.125f,1.0f,3.25f})for(float fov:{0.25f,30.0f,60.0f,89.0f,120.0f}) {
        std::array<float,36> original{};
        original[4]=near_distance;original[5]=100;original[34]=fov;original[35]=fov*0.75f;
        at<Action>(0x4573b0)(original.data());
        sgFrustum native;native.setNearFar(near_distance,100);native.setFOV(fov,fov*0.75f);
        sgMat4 matrix;native.getMat4(matrix);
        compare(!std::memcmp(original.data()+18,matrix,sizeof(matrix)),0x4573b0,"adapted FPTAN frustum projection bits");
    }
}
#ifdef MEGUSTA_ORACLE_NET
static void record_checks() {
    using Transfer=void(__attribute__((thiscall)) *)(void*,const void*);
    for(const char* name:{"", "player", "1234567890123456789012345678901"}) {
        BinaryObject oldplayer,oldgame;
        oldplayer.set(0x2c,0xabcdef12u);
        at<Action>(0x46e000)(oldplayer.pointer());
        at<Action>(0x46e340)(oldgame.pointer());
        netRoomPlayer player;netRoomGame game;
        compare(oldplayer.get(4)==unsigned(player.getID()) && oldplayer.get(0x28)==unsigned(player.game_id) && oldplayer.bytes[0x30]==player.watching && oldplayer.get(0x2c)==0xabcdef12u,0x46e000,"player constructor preserves extra word");
        compare(oldgame.get(4)==unsigned(game.getID()) && oldgame.get(0x28)==unsigned(game.master_id) && oldgame.get(0x2c)==unsigned(game.max_players) && oldgame.get(0x30)==game.protocolValue && oldgame.bytes[0x34]==game.in_progress && oldgame.bytes[0x35]==game.has_password,0x46e340,"game constructor fields");
        std::strcpy(player.name,name);std::strcpy(game.name,name);
        player.game_id=0x1020304;player.watching=true;player.protocolValue=0x89abcdefu;
        game.master_id=7;game.max_players=8;game.protocolValue=0x76543210u;game.in_progress=true;game.has_password=true;
        std::strcpy(reinterpret_cast<char*>(oldplayer.bytes.data()+8),name);oldplayer.set(0x28,player.game_id);oldplayer.bytes[0x30]=1;oldplayer.set(0x2c,player.protocolValue);
        std::strcpy(reinterpret_cast<char*>(oldgame.bytes.data()+8),name);oldgame.set(0x28,game.master_id);oldgame.set(0x2c,game.max_players);oldgame.set(0x30,game.protocolValue);oldgame.bytes[0x34]=1;oldgame.bytes[0x35]=1;
        for(bool isgame:{false,true}) {
            uint32_t put=isgame?0x46e550:0x46e1a0,get=isgame?0x46e360:0x46e020,copy=isgame?0x46e700:0x46e300;
            netMessage native(42,7);std::array<unsigned char,513> wire{};
            std::memcpy(wire.data(),native.getData(),5);
            BinaryObject message;message.set(0,5);message.set(4,512);message.set(8,reinterpret_cast<uintptr_t>(wire.data()));message.set(12,5);
            BinaryObject& record=isgame?oldgame:oldplayer;
            at<Transfer>(put)(record.pointer(),message.pointer());
            if(isgame)game.put(native);else player.put(native);
            compare(message.get(0)==unsigned(native.getLength()) && message.get(12)==unsigned(native.getLength()) && !std::memcmp(wire.data(),native.getData(),native.getLength()),put,"complete golden wire encoding");
            netMessage incoming(native.getData(),native.getLength());
            BinaryObject decoded;message.set(12,5);at<Transfer>(get)(decoded.pointer(),message.pointer());
            netRoomPlayer decodedplayer;netRoomGame decodedgame;
            if(isgame)decodedgame.get(incoming);else decodedplayer.get(incoming);
            compare(message.get(12)==unsigned(native.getLength()) && !std::strcmp(reinterpret_cast<char*>(decoded.bytes.data()+8),name) && decoded.get(isgame?0x30:0x2c)==(isgame?decodedgame.protocolValue:decodedplayer.protocolValue),get,"decode consumes added field in wire order");
            compare(isgame?(decoded.get(0x28)==unsigned(decodedgame.master_id) && decoded.get(0x2c)==unsigned(decodedgame.max_players) && decoded.bytes[0x34]==decodedgame.in_progress && decoded.bytes[0x35]==decodedgame.has_password):(decoded.get(0x28)==unsigned(decodedplayer.game_id) && decoded.bytes[0x30]==decodedplayer.watching),get,"decoded semantic fields");
            BinaryObject copied;copied.set(4,123);at<Transfer>(copy)(copied.pointer(),record.pointer());
            netRoomPlayer copiedplayer;netRoomGame copiedgame;
            if(isgame)copiedgame.copy(&game);else copiedplayer.copy(&player);
            compare(copied.get(4)==123 && !std::strcmp(reinterpret_cast<char*>(copied.bytes.data()+8),name) && copied.get(isgame?0x30:0x2c)==(isgame?copiedgame.protocolValue:copiedplayer.protocolValue),copy,"copy preserves destination id and extra word");
        }
    }
}
#endif
#ifdef MEGUSTA_ORACLE_PUI
static void font_checks() {
    using GlyphSet=void(__attribute__((thiscall)) *)(void*,unsigned,float,float,float,float,float,float,float,float,float);
    using GlyphGet=int(__attribute__((thiscall)) *)(void*,unsigned,float*,float*,float*,float*,float*,float*,float*,float*,float*);
    using Bounds=void(__attribute__((thiscall)) *)(void*,const char*,float,float,float*,float*,float*,float*);
    using Metric=int(__attribute__((thiscall)) *)(void*,const char*);
    using FontMetric=int(__attribute__((thiscall)) *)(void*);
    alignas(4) std::array<unsigned char,10264> raw{};
    uint32_t vtable=0x4ea4b0;std::memcpy(raw.data(),&vtable,4);
    fntTexFont native;native.setWidth(0.75f);native.setGap(0.125f);
    float width=0.75f,gap=0.125f;std::memcpy(raw.data()+16,&width,4);std::memcpy(raw.data()+20,&gap,4);
    for(unsigned c:{unsigned('K'),unsigned('y'),255u}) {
        std::array<float,9> fields={0,1,0,1,-0.125f,0.5f,-0.25f,1,0.625f},old{},now{};
        at<GlyphSet>(0x45bd20)(raw.data(),c,fields[0],fields[1],fields[2],fields[3],fields[4],fields[5],fields[6],fields[7],fields[8]);
        native.setGlyph(char(c),fields[0],fields[1],fields[2],fields[3],fields[4],fields[5],fields[6],fields[7],fields[8]);
        int a=at<GlyphGet>(0x45bda0)(raw.data(),c,&old[0],&old[1],&old[2],&old[3],&old[4],&old[5],&old[6],&old[7],&old[8]);
        int b=native.getGlyph(char(c),&now[0],&now[1],&now[2],&now[3],&now[4],&now[5],&now[6],&now[7],&now[8]);
        compare(a==b && old==fields && now==fields,0x45bd20,"nine glyph fields including advance");
    }
    for(int fixed:{0,1})for(float size:{8.0f,16.0f,20.0f})for(float slant:std::array<float,3>{float(-0.25f),0.0f,0.125f}) {
        native.setFixedPitch(fixed);std::memcpy(raw.data()+12,&fixed,4);
        puFont metrics(&native,size,slant);BinaryObject font;
        font.set(0,reinterpret_cast<uintptr_t>(raw.data()));std::memcpy(font.bytes.data()+4,&size,4);std::memcpy(font.bytes.data()+8,&slant,4);
        for(const char* text:{"","KK","k y","Ky\nK","\xffK"}) {
            std::array<float,4> old{},now{};
            at<Bounds>(0x45be60)(raw.data(),text,size,slant,&old[0],&old[1],&old[2],&old[3]);
            native.getBBox(text,size,slant,&now[0],&now[1],&now[2],&now[3]);
            compare(!std::memcmp(old.data(),now.data(),sizeof(old)),0x45be60,"bitwise font bounds");
            compare(at<Metric>(0x4654c0)(font.pointer(),text)==metrics.getStringWidth(text),0x4654c0,"font width metric");
            compare(at<Metric>(0x465510)(font.pointer(),text)==metrics.getStringHeight(text),0x465510,"font text height metric");
        }
        compare(at<FontMetric>(0x465560)(font.pointer())==metrics.getStringHeight(),0x465560,"font capital height");
        compare(at<FontMetric>(0x465570)(font.pointer())==metrics.getStringDescender(),0x465570,"font descender");
        puButton widget(10,20,110,50);widget.setLabelFont(metrics);widget.setLabel("Ky");
        for(int placement=0;placement<9;++placement) {
            alignas(4) std::array<unsigned char,512> object{};
            const int active[]={10,20,110,50};std::memcpy(object.data()+232,active,sizeof(active));
            uint32_t label=reinterpret_cast<uintptr_t>("Ky");std::memcpy(object.data()+364,&label,4);
            std::memcpy(object.data()+368,font.bytes.data(),12);std::memcpy(object.data()+380,&placement,4);
            at<Action>(0x461380)(object.data());widget.setLabelPlace(placement);
            auto* bounds=widget.getBBox();std::array<int,4> now={bounds->min[0],bounds->min[1],bounds->max[0],bounds->max[1]};
            compare(!std::memcmp(object.data()+216,now.data(),sizeof(now)),0x461380,"four label placements and unsupported no-op cases");
        }
    }
}
static void palette_checks() {
    using Derive=void(__cdecl *)(float*,float,float,float,float);
    puButton native(0,0,1,1);
    const float values[][4]={{0,0,0,1},{1,1,1,0.5f},{0.76923072f,0.5f,-0.2f,0.3f},
      {0.76923078f,0.4f,0.1f,1},{0.5f,0.5f,0.5f,1},{0.1f,0.8f,0.3f,0.7f}};
    for(auto &v:values){
        float original[20]={};at<Derive>(0x460f10)(original,v[0],v[1],v[2],v[3]);
        native.setColourScheme(v[0],v[1],v[2],v[3]);
        for(int slot:{0,1,2,4}){
            float actual[4];native.getColour(slot,&actual[0],&actual[1],&actual[2],&actual[3]);
            compare(!std::memcmp(actual,original+slot*4,sizeof(actual)),0x460f10,"PUI palette float fields");
        }
    }
}
#endif
// Version-0 comparisons execute original serializer code. Only file/allocator
// service boundaries are redirected, avoiding the uninitialized original CRT.
namespace v0_oracle {
static std::set<void*> allocations;
static std::set<FILE*> files;
static void* __cdecl allocate(size_t n){void* p=std::malloc(n?n:1);if(p)allocations.insert(p);return p;}
static void __cdecl release(void* p){if(p){allocations.erase(p);std::free(p);}}
static FILE* __cdecl open(const char* path,const char* mode){FILE* f=std::fopen(path,mode);if(f)files.insert(f);return f;}
static int __cdecl close(FILE* f){files.erase(f);return std::fclose(f);}
static void __cdecl diagnostic(int severity,const char* message,...){if(severity==2)std::abort();std::fprintf(stderr,"original SSG diagnostic(%d): %s\n",severity,message);}
struct MemoryPatch {
    uintptr_t address;std::vector<unsigned char> saved;
    MemoryPatch(uintptr_t a,const void* bytes,size_t count):address(a),saved(count){
        std::memcpy(saved.data(),reinterpret_cast<void*>(a),count);write(bytes);
    }
    void write(const void* bytes){DWORD old,ignored;if(!VirtualProtect(reinterpret_cast<void*>(address),saved.size(),PAGE_EXECUTE_READWRITE,&old))throw std::runtime_error("Cannot patch oracle service boundary");std::memcpy(reinterpret_cast<void*>(address),bytes,saved.size());VirtualProtect(reinterpret_cast<void*>(address),saved.size(),old,&ignored);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),saved.size());}
    ~MemoryPatch(){write(saved.data());}
};
struct Services {
    std::vector<std::unique_ptr<MemoryPatch>> patches;
    template<class T>void cell(uintptr_t address,T function){uintptr_t value=reinterpret_cast<uintptr_t>(function);patches.emplace_back(new MemoryPatch(address,&value,4));}
    template<class T>void jump(uintptr_t address,T function){unsigned char code[5]={0xe9,0,0,0,0};uint32_t displacement=uint32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(code+1,&displacement,4);patches.emplace_back(new MemoryPatch(address,code,5));}
    Services(){
        cell(0x524658,open);cell(0x524644,close);cell(0x524674,&std::fwrite);cell(0x524668,&std::fread);
        cell(0x524664,&std::fputs);cell(0x524660,&std::fputc);cell(0x524650,&std::fgetc);
        cell(0x5243e4,allocate);cell(0x5243f4,release);jump(0x44d78c,allocate);jump(0x44d781,release);
        jump(0x469d40,diagnostic);
    }
    ~Services(){
        for(FILE* file:files)std::fclose(file);files.clear();
        for(void* p:allocations)std::free(p);allocations.clear();
        // Discard the original reader's non-owning global key index after cleanup.
        *reinterpret_cast<uint32_t*>(0x54a070)=0;*reinterpret_cast<uint32_t*>(0x54a06c)=0;
    }
};
struct Raw {
    alignas(4) std::array<unsigned char,256> bytes{};
    std::vector<void*> children;
    void set(unsigned offset,uint32_t value){std::memcpy(bytes.data()+offset,&value,4);}
    void floats(unsigned offset,const float* values,size_t n){std::memcpy(bytes.data()+offset,values,n*4);}
    void* pointer(){return bytes.data();}
    void name(const char* name){set(20,reinterpret_cast<uintptr_t>(name));}
    Raw(uint32_t vtable,uint32_t tag,const char* name){set(0,vtable);set(12,tag);set(48,0xffffffffu);this->name(name);}
    void add(Raw& child){children.push_back(child.pointer());set(84,children.size());set(88,children.size());set(96,reinterpret_cast<uintptr_t>(children.data()));}
};
static std::vector<char> bytes(const char* path){std::ifstream f(path,std::ios::binary);return std::vector<char>((std::istreambuf_iterator<char>(f)),{});}
static void array(Raw& raw,uint32_t vtable,unsigned size,unsigned count,void* data){raw.set(0,vtable);raw.set(12,1);raw.set(28,count);raw.set(32,count);raw.set(36,size);raw.set(40,reinterpret_cast<uintptr_t>(data));}
static std::vector<char> stream_bytes(FILE* file){std::fflush(file);std::rewind(file);std::vector<char> result;char buffer[256];size_t count;while((count=std::fread(buffer,1,sizeof(buffer),file)))result.insert(result.end(),buffer,buffer+count);return result;}
static void reset_original_read(){release(*reinterpret_cast<void**>(0x54a070));*reinterpret_cast<uint32_t*>(0x54a070)=0;*reinterpret_cast<uint32_t*>(0x54a06c)=0;*reinterpret_cast<uint32_t*>(0x54a074)=0;}
static void selector_payload(){
    using Body=int(__attribute__((thiscall)) *)(void*,FILE*);
    Raw state(0x4ebbb4,0xd,"step"),selector(0x4ec2e0,0x1d,"selector-state");
    state.set(36,0xffffffff);state.set(52,0x74);state.set(120,0x3d);
    selector.set(36,0xffffffff);selector.set(52,0x74);selector.set(120,0x3d);
    std::array<void*,3> entries={state.pointer(),state.pointer(),nullptr};selector.set(132,3);selector.set(136,0);selector.set(140,reinterpret_cast<uintptr_t>(entries.data()));
    using File=std::unique_ptr<FILE,decltype(&std::fclose)>;
    File original(std::fopen("v0-selector-input.ssg","w+b"),&std::fclose),native_output(std::fopen("v0-selector-native.ssg","w+b"),&std::fclose),original_output(std::fopen("v0-selector-original.ssg","w+b"),&std::fclose);
    if(!original || !native_output || !original_output)throw std::runtime_error("Cannot open serializer payload streams");
    *reinterpret_cast<uint32_t*>(0x54a074)=0;
    compare((at<Body>(0x49a220)(selector.pointer(),original.get())&0xff)!=0,0x49a220,"state-selector payload write");
    std::rewind(original.get());_ssgV0ResetReadContext();ssgStateSelector native;
    compare(native.load(original.get())!=0,0x49a030,"native accepts original state-selector payload");
    reset_original_read();void* loaded=allocate(0x90);at<PtrGet>(0x499b70)(loaded);
    std::rewind(original.get());compare((at<Body>(0x49a030)(loaded,original.get())&0xff)!=0,0x49a030,"original reloads state-selector payload");
    uintptr_t steps;std::memcpy(&steps,static_cast<unsigned char*>(loaded)+140,4);auto* pointers=reinterpret_cast<void**>(steps);
    compare(pointers[0] && pointers[0]==pointers[1] && !pointers[2] && native.getStep(0)==native.getStep(1),0x49a030,"state-selector shared and null step identity");
    *reinterpret_cast<uint32_t*>(static_cast<unsigned char*>(pointers[0])+16)=0;
    *reinterpret_cast<uint32_t*>(static_cast<unsigned char*>(loaded)+16)=0;
    *reinterpret_cast<uint32_t*>(0x54a074)=0;
    native.getStep(0)->zeroSpare();native.ssgBase::zeroSpare();_ssgV0ResetReadContext();
    at<Body>(0x49a220)(loaded,original_output.get());native.save(native_output.get());
    compare(stream_bytes(original_output.get())==stream_bytes(native_output.get()),0x49a220,"state-selector original/native payload bytes after readback");
}
static void texture_payload(){
    class MetadataTexture : public ssgTexture {public:void parameters(){wrapu=0;wrapv=1;mipmap=0;}};
    using Body=int(__attribute__((thiscall)) *)(void*,FILE*);
    for(const char* filename:std::array<const char*,3>{nullptr,"sample.tga","folder/picture.png"}) {
        MetadataTexture native;native.parameters();native.setFilename(filename);native.setName("texture");
        Raw original(0x4ec068,0x20001,"texture");original.set(32,reinterpret_cast<uintptr_t>(filename));original.set(40,0);original.set(44,1);original.set(48,0);
        using File=std::unique_ptr<FILE,decltype(&std::fclose)>;
        File old(std::fopen("v0-texture-original.ssg","w+b"),&std::fclose),now(std::fopen("v0-texture-native.ssg","w+b"),&std::fclose);
        if(!old || !now)throw std::runtime_error("Cannot open texture payload streams");
        at<Body>(0x48e0c0)(original.pointer(),old.get());native.save(now.get());
        compare(stream_bytes(old.get())==stream_bytes(now.get()),0x48e0c0,"texture filename/wrap/mipmap/base payload bytes; no image loading");
    }
}
static void reader_edges(){
    using Load=void*(__cdecl *)(const char*);using Save=int(__cdecl *)(const char*,void*);
    auto word=[](std::vector<char>& data,unsigned offset,uint32_t value){std::memcpy(data.data()+offset,&value,4);};
    auto write=[](const std::vector<char>& data){std::ofstream f("v0-edge.ssg",std::ios::binary);f.write(data.data(),data.size());};
    ssgRoot root;root.setName("root");ssgSaveSSG("v0-edge-base.ssg",&root);auto baseline=bytes("v0-edge-base.ssg");
    for(uint32_t magic:{0x53534701u,0x53534702u,0x00475353u,0xdeadbeefu}) {
        auto data=baseline;word(data,0,magic);write(data);
        void* original=at<Load>(0x48b5b0)("v0-edge.ssg");std::unique_ptr<ssgEntity> native(ssgLoadSSG("v0-edge.ssg"));
        compare(!original && !native,0x48b5b0,"unsupported version/magic rejection");
    }
    {auto data=baseline;word(data,4,0x213);write(data);compare(!at<Load>(0x48b5b0)("v0-edge.ssg") && !ssgLoadSSG("v0-edge.ssg"),0x48b5b0,"unsupported entity type rejected");}
    ssgVtxTable bare(GL_TRIANGLES,new ssgVertexArray,new ssgNormalArray,new ssgTexCoordArray,new ssgColourArray);bare.setName("bare");bare.recalcBSphere();
    ssgSaveSSG("v0-edge-base.ssg",&bare);baseline=bytes("v0-edge-base.ssg");
    for(bool unknown_state:{false,true}) {
        auto data=baseline;
        if(unknown_state){word(data,48,0xdead);data.erase(data.begin()+52,data.begin()+56);}
        else {word(data,36,0xdecafbad);word(data,56,0x10406080);}
        write(data);void* original=at<Load>(0x48b5b0)("v0-edge.ssg");std::unique_ptr<ssgEntity> native(ssgLoadSSG("v0-edge.ssg"));
        compare(original && native,0x48b5b0,unknown_state?"unknown state tag continues as null":"record markers are ignored as in original");
        if(original && native){at<Save>(0x48b940)("v0-edge-original.ssg",original);ssgSaveSSG("v0-edge-native.ssg",native.get());compare(bytes("v0-edge-original.ssg")==bytes("v0-edge-native.ssg"),0x48b5b0,"edge-case decoded payloads agree");}
    }
}
static void run(){
    Services services;
    using Save=int(__cdecl *)(const char*,void*);
    using Load=void*(__cdecl *)(const char*);
    for(unsigned variant=0;variant<4;++variant){
        ssgRoot root;root.setName("root");Raw oldroot(0x4eb958,0x413,"root");
        Raw branch(0x4eb77c,0x13,"branch"),transform(0x4ebddc,0x73,"transform"),selector(0x4ebeac,0x113,"selector"),cutout(0x4ec110,0x813,"cutout"),textrans(0x4ec0bc,0x2013,"textrans");
        Raw geometry(variant==2?0x4ec164:0x4ebf7c,variant==2?0xc007:0x4007,"mesh");
        Raw second_geometry(0x4ebf7c,0x4007,"second"),state(0x4ebbb4,0xd,"state");
        Raw vertices(0,1,""),normals(0,1,""),uv(0,1,""),colors(0,1,""),indices(0,1,"");
        sgVec3 positions[3]={{0,0,0},{1,0,0},{0,1,0}};short index_values[3]={0,1,2};
        if(variant){
            auto* native_branch=new ssgBranch;native_branch->setName("branch");root.addKid(native_branch);oldroot.add(branch);
            if(variant==3){root.addKid(native_branch);oldroot.add(branch);}
            else {
                auto* v=new ssgVertexArray;for(auto& point:positions)v->add(point);
                auto* n=new ssgNormalArray;auto* t=new ssgTexCoordArray;auto* c=new ssgColourArray;ssgVtxTable* mesh;
                if(variant==2){auto* ix=new ssgIndexArray;for(short index:index_values)ix->add(index);mesh=new ssgVtxArray(GL_TRIANGLES,v,n,t,c,ix);geometry.set(164,reinterpret_cast<uintptr_t>(indices.pointer()));}
                else mesh=new ssgVtxTable(GL_TRIANGLES,v,n,t,c);
                mesh->setName("mesh");mesh->setRenderOrderMode(7);mesh->recalcBSphere();native_branch->addKid(mesh);branch.add(geometry);
                const float bounds[6]={0,0,0,1,1,0};geometry.floats(120,bounds,6);geometry.set(144,8);geometry.set(80,1);geometry.set(84,7);
                array(vertices,0x4eb01c,12,3,positions);array(normals,0x4eb8ec,12,0,nullptr);array(uv,0x4eb8b8,8,0,nullptr);array(colors,0x4eafe8,16,0,nullptr);array(indices,0x4eb884,2,3,index_values);
                geometry.set(148,reinterpret_cast<uintptr_t>(vertices.pointer()));geometry.set(152,reinterpret_cast<uintptr_t>(normals.pointer()));geometry.set(156,reinterpret_cast<uintptr_t>(uv.pointer()));geometry.set(160,reinterpret_cast<uintptr_t>(colors.pointer()));
                auto* material=new ssgSimpleState;material->setName("state");material->setTranslucent();material->setExternalPropertyIndex(9);
                const GLenum modes[]={GL_SPECULAR,GL_EMISSION,GL_AMBIENT,GL_DIFFUSE};
                for(unsigned color=0;color<4;++color){sgVec4 rgba={float(float(color)*0.25f),0.5f,0.75f,1};material->setMaterial(modes[color],rgba);state.floats(56+color*16,rgba,4);}
                material->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);material->setShadeModel(GL_SMOOTH);material->setShininess(8);material->setAlphaClamp(0.25f);
                material->enable(GL_LIGHTING);material->disable(GL_BLEND);mesh->setState(material);
                uint32_t care=0;for(unsigned bit=0;bit<32;++bit)if(material->getCareAbout(bit))care|=1u<<bit;
                state.set(28,1);state.set(32,9);state.set(36,care);state.set(40,0x20);state.set(52,0x74);state.set(120,0x3d);
                const float scalars[]={8,0.25f};state.floats(124,scalars,2);geometry.set(88,reinterpret_cast<uintptr_t>(state.pointer()));
                if(variant==2){
                    second_geometry.bytes=geometry.bytes;second_geometry.set(0,0x4ebf7c);second_geometry.set(12,0x4007);second_geometry.name("second");branch.add(second_geometry);
                    auto* second=new ssgVtxTable(GL_TRIANGLES,v,n,t,c);second->setName("second");second->setRenderOrderMode(7);second->setState(material);second->recalcBSphere();native_branch->addKid(second);
                }
                auto* tr=new ssgTransform;tr->setName("transform");sgMat4 matrix;sgMakeTransMat4(matrix,2,3,4);tr->setTransform(matrix);root.addKid(tr);oldroot.add(transform);transform.floats(100,&matrix[0][0],16);
                auto* choices=new ssgSelector(4);choices->setName("selector");root.addKid(choices);oldroot.add(selector);std::array<unsigned char,4> selection={1,1,1,1};selector.set(100,reinterpret_cast<uintptr_t>(selection.data()));selector.set(104,4);
                auto* co=new ssgCutout(1);co->setName("cutout");root.addKid(co);oldroot.add(cutout);cutout.set(100,1);
                auto* tx=new ssgTexTrans;tx->setName("textrans");root.addKid(tx);oldroot.add(textrans);
                // Keep borrowed selector storage alive through both writes below.
                at<Save>(0x48b940)("v0-original.ssg",oldroot.pointer());ssgSaveSSG("v0-native.ssg",&root);
                {auto old=bytes("v0-original.ssg"),now=bytes("v0-native.ssg");if(old!=now){size_t offset=0;while(offset<old.size() && offset<now.size() && old[offset]==now[offset])++offset;std::fprintf(stderr,"v0 variant%u first difference at%u original=%02x native=%02x lengths%u/%u\n",variant,unsigned(offset),offset<old.size()?unsigned((unsigned char)old[offset]):0,offset<now.size()?unsigned((unsigned char)now[offset]):0,unsigned(old.size()),unsigned(now.size()));}}
                compare(bytes("v0-original.ssg")==bytes("v0-native.ssg"),0x48b940,"version0 complete nested file bytes");
                void* original_loaded=at<Load>(0x48b5b0)("v0-native.ssg");
                std::unique_ptr<ssgEntity> native_loaded(ssgLoadSSG("v0-original.ssg"));
                compare(original_loaded && native_loaded,0x48b5b0,"both readers accept nested version0 file");
                if(variant==2 && original_loaded && native_loaded){
                    auto get=[](void* p,unsigned offset){uintptr_t value;std::memcpy(&value,static_cast<unsigned char*>(p)+offset,4);return reinterpret_cast<void*>(value);};
                    void* old_branch=*static_cast<void**>(get(original_loaded,96));void** old_meshes=static_cast<void**>(get(old_branch,96));
                    void* first_state=get(old_meshes[0],88);void* second_state=get(old_meshes[1],88);
                    auto* group=static_cast<ssgBranch*>(static_cast<ssgBranch*>(native_loaded.get())->getKid(0));
                    auto* first=static_cast<ssgLeaf*>(group->getKid(0));auto* second=static_cast<ssgLeaf*>(group->getKid(1));
                    using Lookup=void*(__cdecl *)(int);uint32_t key=0;if(first_state)std::memcpy(&key,static_cast<unsigned char*>(first_state)+16,4);
                    compare(first_state && first_state==second_state && first->getState()==second->getState() && at<Lookup>(0x48b4a0)(key)==first_state,0x48b4a0,"shared state key lookup and alias identity");
                }
                if(original_loaded && native_loaded){at<Save>(0x48b940)("v0-original-round.ssg",original_loaded);ssgSaveSSG("v0-native-round.ssg",native_loaded.get());compare(bytes("v0-original-round.ssg")==bytes("v0-native-round.ssg"),0x48b5b0,"original/native readback and resave bytes");}
                continue;
            }
        }
        int original_okay=at<Save>(0x48b940)("v0-original.ssg",oldroot.pointer());int native_okay=ssgSaveSSG("v0-native.ssg",&root);
        compare(original_okay && native_okay && bytes("v0-original.ssg")==bytes("v0-native.ssg"),0x48b940,"empty/shared version0 complete file bytes");
        if(variant==0){void* loaded=at<Load>(0x48b5b0)("v0-native.ssg");compare(loaded!=nullptr,0x48b5b0,"original reader accepts native empty root");}
    }
    selector_payload();
    texture_payload();
    reader_edges();
}
}
#include "original_state_table_addresses.h"
namespace io_oracle {
static void base_objects(){
    struct Base: ssgBase {using ssgBase::copy_from;};
    BinaryObject raw,source;Base native,native_source;
    at<PtrGet>(0x4666e0)(raw.pointer());at<PtrGet>(0x4666e0)(source.pointer());
    compare(raw.get(4)==0 && raw.get(12)==uint32_t(native.getType()) && raw.get(16)==0 && raw.get(20)==0 && raw.get(24)==0,0x4666e0,"base constructor fields, excluding global unique counter");
    compare(source.get(8)==raw.get(8)+1,0x4666e0,"base unique counter increments");
    for(uint32_t value:{0u,1u,0x7ffffffeu,0xfffffffeu}){
        at<Set>(0x466820)(raw.pointer(),value);native.setSpare(int(value));
        compare(at<Get>(0x466830)(raw.pointer())==uint32_t(native.getSpare()),0x466820,"base spare assignment");
        at<Action>(0x466810)(raw.pointer());native.incSpare();
        compare(at<Get>(0x466830)(raw.pointer())==uint32_t(native.getSpare()),0x466810,"base spare increment");
        at<Action>(0x466800)(raw.pointer());native.zeroSpare();
        compare(at<Get>(0x466830)(raw.pointer())==uint32_t(native.getSpare()),0x466800,"base spare reset");
        at<Set>(0x466820)(raw.pointer(),123);native.setSpare(123);
        at<Action>(0x4667f0)(raw.pointer());native.zeroSpareRecursive();
        compare(at<Get>(0x466830)(raw.pointer())==uint32_t(native.getSpare()),0x4667f0,"base recursive reset virtual dispatch");
    }
    using Name=void(__attribute__((thiscall))*)(void*,const char*);
    using Copy=void(__attribute__((thiscall))*)(void*,void*,int);
    for(const char* name:{static_cast<const char*>(nullptr),"","scene","changed-name"}){
        at<Name>(0x4667b0)(source.pointer(),name);native_source.setName(name);
        const char* old=reinterpret_cast<const char*>(source.get(20));
        compare((!old&&!native_source.getName())||(old&&native_source.getName()&&!std::strcmp(old,native_source.getName())),0x4667b0,"name replacement and null identity");
        for(int flags:{0,SSG_CLONE_USERDATA}){
            source.set(16,37);native_source.setSpare(37);
            at<Copy>(0x466620)(raw.pointer(),source.pointer(),flags);native.copy_from(&native_source,flags);
            old=reinterpret_cast<const char*>(raw.get(20));
            compare(raw.get(16)==uint32_t(native.getSpare()) && raw.get(4)==uint32_t(native.getRef()) &&
                    ((!old&&!native.getName())||(old&&native.getName()&&!std::strcmp(old,native.getName()))) &&
                    raw.get(24)==0 && !native.getUserData(),0x466620,"base copy name, spare, reference count and null userdata");
        }
    }
    at<Action>(0x466740)(raw.pointer());at<Action>(0x466740)(source.pointer());
    compare(raw.get(12)==0xdeadbeefu && source.get(12)==0xdeadbeefu,0x466740,"base destructor poison marker and name release");
    ssgBranch branch;ssgVtxTable mesh;ssgTexture texture;ssgSimpleState state;
    ssgStateSelector selector_state;ssgVtxArray indexed;ssgSelector selector;ssgTransform transform;ssgCutout cutout;ssgRoot root;
    const char* names[]={native.getTypeName(),texture.getTypeName(),state.ssgState::getTypeName(),state.getTypeName(),selector_state.getTypeName(),
        branch.ssgEntity::getTypeName(),mesh.ssgLeaf::getTypeName(),mesh.getTypeName(),indexed.getTypeName(),branch.getTypeName(),selector.getTypeName(),
        transform.ssgBaseTransform::getTypeName(),transform.getTypeName(),cutout.getTypeName(),root.getTypeName()};
    for(unsigned i=0;i<15;++i){uint32_t address=0x45a220+i*16;
        compare(!std::strcmp(static_cast<const char*>(at<PtrGet>(address)(raw.pointer())),names[i]),address,"class type-name string");}
}
using Stream=std::unique_ptr<FILE,decltype(&std::fclose)>;
static Stream stream(const char* name){FILE* f=std::fopen(name,"w+b");if(!f)throw std::runtime_error("Cannot open IO comparison stream");return Stream(f,&std::fclose);}
template<class T> static void scalar(uint32_t write_address,uint32_t read_address,
                                   void(*writer)(FILE*,T),void(*reader)(FILE*,T*)){
    auto original=stream("io-original.bin"),native=stream("io-native.bin");
    for(unsigned i=0;i<128;++i){
        uint32_t bits=random_word();T value;std::memcpy(&value,&bits,4);
        at<void(__cdecl*)(FILE*,T)>(write_address)(original.get(),value);writer(native.get(),value);
    }
    compare(v0_oracle::stream_bytes(original.get())==v0_oracle::stream_bytes(native.get()),write_address,"scalar exact bytes including arbitrary float bit patterns");
    std::rewind(original.get());std::rewind(native.get());
    for(unsigned i=0;i<128;++i){T a{},b{};at<void(__cdecl*)(FILE*,T*)>(read_address)(original.get(),&a);reader(native.get(),&b);
        compare(std::memcmp(&a,&b,4)==0,read_address,"scalar read exact bits");}
}
static void run(){
    v0_oracle::Services services;
    base_objects();
    scalar<float>(0x45a390,0x45a360,static_cast<void(*)(FILE*,float)>(&_ssgWriteFloat),static_cast<void(*)(FILE*,float*)>(&_ssgReadFloat));
    scalar<unsigned>(0x45a3f0,0x45a3c0,static_cast<void(*)(FILE*,unsigned)>(&_ssgWriteUInt),static_cast<void(*)(FILE*,unsigned*)>(&_ssgReadUInt));
    scalar<int>(0x45a450,0x45a420,static_cast<void(*)(FILE*,int)>(&_ssgWriteInt),static_cast<void(*)(FILE*,int*)>(&_ssgReadInt));
    for(unsigned count:{0u,1u,3u,16u,63u}){
        auto original=stream("io-original.bin"),native=stream("io-native.bin");
        std::array<float,64> values{},a{},b{};for(float& v:values){uint32_t bits=random_word();std::memcpy(&v,&bits,4);}
        at<void(__cdecl*)(FILE*,unsigned,const float*)>(0x45a4b0)(original.get(),count,values.data());_ssgWriteFloat(native.get(),count,values.data());
        compare(v0_oracle::stream_bytes(original.get())==v0_oracle::stream_bytes(native.get()),0x45a4b0,"float-array exact bytes");
        std::rewind(original.get());std::rewind(native.get());
        at<void(__cdecl*)(FILE*,unsigned,float*)>(0x45a480)(original.get(),count,a.data());_ssgReadFloat(native.get(),count,b.data());
        compare(!std::memcmp(a.data(),b.data(),sizeof(a)),0x45a480,"float-array read including zero count");
        original=stream("io-original.bin");native=stream("io-native.bin");
        at<void(__cdecl*)(FILE*,unsigned,const void*)>(0x45a510)(original.get(),count,values.data());_ssgWriteBytes(native.get(),count,values.data());
        compare(v0_oracle::stream_bytes(original.get())==v0_oracle::stream_bytes(native.get()),0x45a510,"raw-byte exact output");
        std::rewind(original.get());std::rewind(native.get());a={};b={};
        at<void(__cdecl*)(FILE*,unsigned,void*)>(0x45a4e0)(original.get(),count,a.data());_ssgReadBytes(native.get(),count,b.data());
        compare(!std::memcmp(a.data(),b.data(),sizeof(a)),0x45a4e0,"raw-byte read including zero count");
    }
    using Vector=void(__cdecl*)(FILE*,float*);
    for(unsigned dimension:{3u,4u,16u}){
        auto original=stream("io-original.bin"),native=stream("io-native.bin");
        std::array<float,16> values{},a{},b{};for(float& v:values){uint32_t bits=random_word();std::memcpy(&v,&bits,4);}
        uint32_t write_address=dimension==3?0x45a620:dimension==4?0x45a660:0x45a6a0;
        uint32_t read_address=write_address-0x20;
        at<Vector>(write_address)(original.get(),values.data());
        if(dimension==3)_ssgWriteVec3(native.get(),values.data());else if(dimension==4)_ssgWriteVec4(native.get(),values.data());else _ssgWriteMat4(native.get(),reinterpret_cast<float(*)[4]>(values.data()));
        compare(v0_oracle::stream_bytes(original.get())==v0_oracle::stream_bytes(native.get()),write_address,"vector/matrix output exact bits");
        std::rewind(original.get());std::rewind(native.get());at<Vector>(read_address)(original.get(),a.data());
        if(dimension==3)_ssgReadVec3(native.get(),b.data());else if(dimension==4)_ssgReadVec4(native.get(),b.data());else _ssgReadMat4(native.get(),reinterpret_cast<float(*)[4]>(b.data()));
        compare(!std::memcmp(a.data(),b.data(),sizeof(a)),read_address,"vector/matrix input exact bits");
    }
    for(const char* text:{static_cast<const char*>(nullptr),"","scene-root","longer path/texture.bmp"}){
        auto original=stream("io-original.bin"),native=stream("io-native.bin");
        at<void(__cdecl*)(FILE*,const char*)>(0x45a5d0)(original.get(),text);_ssgWriteString(native.get(),text);
        compare(v0_oracle::stream_bytes(original.get())==v0_oracle::stream_bytes(native.get()),0x45a5d0,"string wire bytes and null encoding");
        std::rewind(original.get());std::rewind(native.get());char* a=nullptr;char* b=nullptr;
        at<void(__cdecl*)(FILE*,char**)>(0x45a540)(original.get(),&a);_ssgReadString(native.get(),&b);
        compare((!a&&!b)||(a&&b&&!std::strcmp(a,b)),0x45a540,"string read allocation and null normalization");
        v0_oracle::release(a);delete[] b;
    }
    using Equal=int(__cdecl*)(const char*,const char*,int);
    for(const char* a:{static_cast<const char*>(nullptr),"","a","ASE","texture.SSG","MiXeD"})
    for(const char* b:{static_cast<const char*>(nullptr),"","A","ase","texture.ssg","mixed","different"})
    for(int length:{-1,0,1,3,5,11})compare(at<Equal>(0x45a6c0)(a,b,length)==ulStrNEqual(a,b,length),0x45a6c0,"case-insensitive prefix boundary semantics");
    compare(at<int(__cdecl*)()>(0x45a340)()==_ssgReadError(),0x45a340,"successful IO read-error state");
    compare(at<int(__cdecl*)()>(0x45a350)()==_ssgWriteError(),0x45a350,"successful IO write-error state");
}
}
namespace state_table_oracle {
static std::vector<uint32_t> calls;
static void APIENTRY enable(GLenum value){calls.push_back(value);}
static void APIENTRY disable(GLenum value){calls.push_back(value|0x80000000u);}
// Patch import cells, leaving original engine enum adapters and all table bodies
// intact. Both modules call the same capture functions without an OpenGL context.
static void imports(HMODULE module,const char* name,void* replacement,
                    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>>& patches){
    auto* base=reinterpret_cast<unsigned char*>(module);
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    auto rva=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    bool found=false;
    for(auto* descriptor=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base+rva);descriptor->Name;++descriptor){
        if(!descriptor->OriginalFirstThunk)continue;
        auto* names=reinterpret_cast<IMAGE_THUNK_DATA32*>(base+descriptor->OriginalFirstThunk);
        auto* slots=reinterpret_cast<IMAGE_THUNK_DATA32*>(base+descriptor->FirstThunk);
        for(;names->u1.AddressOfData;++names,++slots){
            if(IMAGE_SNAP_BY_ORDINAL32(names->u1.Ordinal))continue;
            auto* entry=reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base+names->u1.AddressOfData);
            if(std::strcmp(reinterpret_cast<char*>(entry->Name),name))continue;
            uint32_t address=reinterpret_cast<uintptr_t>(replacement);
            patches.emplace_back(new v0_oracle::MemoryPatch(reinterpret_cast<uintptr_t>(&slots->u1.Function),&address,4));
            found=true;
        }
    }
    if(!found)throw std::runtime_error("Missing OpenGL capture import");
}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE own=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCSTR>(&run),&own))throw std::runtime_error("Cannot locate oracle DLL");
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
        imports(module,"glEnable",reinterpret_cast<void*>(&enable),patches);
        imports(module,"glDisable",reinterpret_cast<void*>(&disable),patches);
    }
    BinaryObject raw;
    uint32_t pointer=reinterpret_cast<uintptr_t>(raw.pointer());
    v0_oracle::MemoryPatch context(0x53db60,&pointer,4);
    ssgContext* saved=_ssgCurrentContext;
    {
        ssgContext native;
        const uint32_t tokens[]={GL_TEXTURE_2D,GL_CULL_FACE,GL_COLOR_MATERIAL,GL_BLEND,GL_ALPHA_TEST,GL_LIGHTING};
        for(unsigned overrides=0;overrides<4;++overrides){
            raw.set(84,overrides&1);raw.set(88,(overrides>>1)&1);
            native.overrideTexture(overrides&1);native.overrideCullface((overrides>>1)&1);
            for(unsigned operation=0;operation<2;++operation)for(unsigned mask=1;mask<64;++mask){
                const uint32_t address=operation?original_disable[mask]:original_enable[mask];
                calls.clear();at<void(__cdecl*)()>(address)();auto original=calls;
                calls.clear();(operation?__ssgDisableTable:__ssgEnableTable)[mask]();
                compare(original==calls,address,"ordered GL state-table calls");
                std::vector<uint32_t> expected;
                for(unsigned bit=0;bit<6;++bit)if((mask&(1u<<bit)) &&
                   (operation || bit>1 || !(overrides&(1u<<bit))))
                    expected.push_back(tokens[bit]|(operation?0x80000000u:0));
                compare(calls==expected,address,"complete state-table mask and override semantics");
            }
        }
    }
    _ssgCurrentContext=saved;
}
}
namespace cutout_oracle {
static sgMat4 captured;
static unsigned loads;
static void APIENTRY matrix(const GLfloat* value){std::memcpy(captured,value,sizeof(captured));++loads;}
static void APIENTRY stack(){}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE own=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&run),&own))throw std::runtime_error("Cannot locate cutout oracle");
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
        state_table_oracle::imports(module,"glLoadMatrixf",reinterpret_cast<void*>(&matrix),patches);
        state_table_oracle::imports(module,"glPushMatrix",reinterpret_cast<void*>(&stack),patches);
        state_table_oracle::imports(module,"glPopMatrix",reinterpret_cast<void*>(&stack),patches);
    }
    using Cull=void(__attribute__((thiscall)) *)(void*,sgFrustum*,float*,int);
    for(int point=0;point<2;++point)for(int i=1;i<=64;++i){
        sgMat4 input;
        for(int column=0;column<4;++column)for(int row=0;row<4;++row)
            input[column][row]=float((i*17+column*31+row*7)%101-50)/float(i+3);
        v0_oracle::Raw raw(0x4ec110,0x813,"cutout");raw.set(100,point);
        ssgCutout native(point);
        loads=0;at<Cull>(0x48e7e0)(raw.pointer(),nullptr,&input[0][0],0);
        sgMat4 old;std::memcpy(old,captured,sizeof(old));unsigned old_loads=loads;
        loads=0;native.cull(nullptr,input,0);
        compare(old_loads==1 && loads==1,0x48e7e0,"one cutout modelview load");
        compare(!std::memcmp(old,captured,sizeof(old)),0x48e7e0,"cutout axis scale and retained normalization bits");
    }
}
}
namespace material_oracle {
static std::vector<uint32_t> events;
static uint32_t bits(float value){uint32_t result;std::memcpy(&result,&value,4);return result;}
static void APIENTRY enable(GLenum v){events.insert(events.end(),{1,v});}
static void APIENTRY disable(GLenum v){events.insert(events.end(),{2,v});}
static void APIENTRY colour(GLenum a,GLenum b){events.insert(events.end(),{3,a,b});}
static void APIENTRY scalar(GLenum a,GLenum b,GLfloat c){events.insert(events.end(),{4,a,b,bits(c)});}
static void APIENTRY vector(GLenum a,GLenum b,const GLfloat* c){events.insert(events.end(),{5,a,b,bits(c[0]),bits(c[1]),bits(c[2]),bits(c[3])});}
static void APIENTRY shade(GLenum a){events.insert(events.end(),{6,a});}
static void APIENTRY alpha(GLenum a,GLfloat b){events.insert(events.end(),{7,a,bits(b)});}
static void initialize(v0_oracle::Raw& raw,ssgSimpleState& native,unsigned seed){
    raw.set(36,native.dont_care=~(int(seed*193u)&0x7fbf));
    raw.set(40,native.enables=seed&63);
    raw.set(52,(seed&1)?0x74:0x70);native.colour_material_mode=(seed&1)?GL_AMBIENT_AND_DIFFUSE:GL_AMBIENT;
    sgVec4* vectors[]={&native.specular_colour,&native.emission_colour,&native.ambient_colour,&native.diffuse_colour};
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)(*vectors[i])[j]=float((seed+i*5+j*3)%7)/8;
        raw.floats(56+i*16,*vectors[i],4);}
    raw.set(120,0x3d);native.shade_model=GL_SMOOTH;
    native.shininess=float(seed%13);native.alpha_clamp=float(seed%5)/8;
    raw.floats(124,&native.shininess,1);raw.floats(128,&native.alpha_clamp,1);
}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE own=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&run),&own))throw std::runtime_error("Cannot locate material oracle");
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
        using state_table_oracle::imports;
        imports(module,"glEnable",reinterpret_cast<void*>(&enable),patches);
        imports(module,"glDisable",reinterpret_cast<void*>(&disable),patches);
        imports(module,"glColorMaterial",reinterpret_cast<void*>(&colour),patches);
        imports(module,"glMaterialf",reinterpret_cast<void*>(&scalar),patches);
        imports(module,"glMaterialfv",reinterpret_cast<void*>(&vector),patches);
        imports(module,"glShadeModel",reinterpret_cast<void*>(&shade),patches);
        imports(module,"glAlphaFunc",reinterpret_cast<void*>(&alpha),patches);
    }
    ssgContext* saved=_ssgCurrentContext;
    {
        ssgContext context;
        for(unsigned force=0;force<2;++force)for(unsigned seed=0;seed<128;++seed){
            v0_oracle::Raw oldstate(0x4ebbb4,0xd,"material"),oldcache(0x4ebbb4,0xd,"cache");
            ssgSimpleState native;
            initialize(oldstate,native,seed);initialize(oldcache,*context.getState(),seed/3);
            BinaryObject rawcontext;rawcontext.set(0,reinterpret_cast<uintptr_t>(oldcache.pointer()));
            uint32_t pointer=reinterpret_cast<uintptr_t>(rawcontext.pointer());
            v0_oracle::MemoryPatch current(0x53db60,&pointer,4);
            uint32_t address=force?0x46c690:0x46c310;
            events.clear();at<Action>(address)(oldstate.pointer());auto old=events;
            events.clear();if(force)native.force();else native.apply();
            compare(old==events,address,"ordered material and capability GL calls");
            const float* fields[]={context.getState()->specular_colour,context.getState()->emission_colour,
                context.getState()->ambient_colour,context.getState()->diffuse_colour};
            for(unsigned i=0;i<4;++i)
                compare(!std::memcmp(oldcache.bytes.data()+56+i*16,fields[i],16),address,"material cache including untouched alpha");
            uint32_t old_enables;std::memcpy(&old_enables,oldcache.bytes.data()+40,4);
            compare(old_enables==uint32_t(context.getState()->enables),address,"capability cache");
        }
    }
    _ssgCurrentContext=saved;
}
}
namespace image_oracle {
static std::vector<unsigned char> uploads;
static void APIENTRY integer(GLenum name,GLint* value){*value=name==GL_MAX_TEXTURE_SIZE?4096:4;}
static void APIENTRY upload(GLenum target,GLint level,GLint components,GLsizei width,GLsizei height,
                           GLint border,GLenum format,GLenum type,const void* pixels){
    uint32_t header[]={target,uint32_t(level),uint32_t(components),uint32_t(width),uint32_t(height),uint32_t(border),format,type};
    auto* bytes=reinterpret_cast<const unsigned char*>(header);
    uploads.insert(uploads.end(),bytes,bytes+sizeof(header));
    if(pixels){bytes=static_cast<const unsigned char*>(pixels);uploads.insert(uploads.end(),bytes,bytes+width*height*components);}
}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE own=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&run),&own))throw std::runtime_error("Cannot locate image oracle");
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
        state_table_oracle::imports(module,"glTexImage2D",reinterpret_cast<void*>(&upload),patches);
        state_table_oracle::imports(module,"glGetIntegerv",reinterpret_cast<void*>(&integer),patches);
    }
    // Select the original renderer's non-16-bit upload profile explicitly.
    uint32_t depth=32;v0_oracle::MemoryPatch rendering_depth(0x524620,&depth,4);
    v0_oracle::Services services;
    services.cell(0x52466c,&std::fseek); // Same FILE runtime as the redirected open/read.
    for(unsigned variant=0;variant<12;++variant){
        bool rgba=variant%3!=0,top=variant&1;
        unsigned stride=rgba?4:3;
        std::vector<unsigned char> file(18+256*stride+8);
        file[1]=file[2]=1;file[6]=1;file[7]=stride*8;file[12]=4;file[14]=2;file[16]=8;file[17]=top?32:0;
        for(unsigned i=0;i<256;++i){file[18+i*stride]=i;file[19+i*stride]=255-i;file[20+i*stride]=i^0x55;
            if(rgba)file[21+i*stride]=(variant%3==1)?255:((i&1)?127:0);}
        for(unsigned i=0;i<8;++i)file[18+256*stride+i]=(i*31+variant*7)&255;
        if(variant>=9)file[2]=2; // Unsupported true-colour header takes fallback.
        const char* path="paletted-original-comparison.tga";
        {FILE* output=std::fopen(path,"wb");if(!output)throw std::runtime_error("Cannot write TGA fixture");
            std::fwrite(file.data(),1,file.size(),output);std::fclose(output);}
        uploads.clear();at<void(__cdecl*)(const char*)>(0x4b5fe0)(path);auto old=uploads;
        uploads.clear();ssgTextureInfo info{};ssgLoadTGA(path,&info);
        compare(old==uploads,0x4b5fe0,"paletted TGA orientation, alpha and complete mip levels");
        if(variant<9)compare(info.alpha==int(*reinterpret_cast<unsigned char*>(0x53ddde)!=0),0x4b5fe0,"palette-derived alpha flag");
    }
    struct PixLevel {int16_t width,height,compression,padding;uint32_t packed,unpacked;};
    for(unsigned variant=0;variant<12;++variant){
        unsigned components=(variant&1)?4:3,raw_size=components*4;
        unsigned codec=variant<2?0:variant<4?2:variant<6?4:variant<8?3:0;
        unsigned mipmap_mode=variant<8?0:variant<10?1:2;
        std::vector<unsigned char> bytes;
        if(codec==2){
            bytes={0,static_cast<unsigned char>(components+1),static_cast<unsigned char>(128u>>components)};
            for(unsigned c=0;c<components;++c)bytes.push_back(c*47+13);
            unsigned code=((components-1)<<6)|(raw_size-components-3);bytes.push_back(code>>8);bytes.push_back(code);
        }else if(codec==4){
            bytes.push_back(components-1);for(unsigned c=0;c<components;++c)bytes.push_back(c*47+13);
            unsigned length=raw_size-components-2;
            bytes.push_back(std::min(length,7u)<<5);if(length>=7)bytes.push_back(length-7);bytes.push_back(components-1);
        }else for(unsigned i=0;i<raw_size;++i)bytes.push_back((i%components)*47+13);
        bool all_levels=variant&1;unsigned levels=mipmap_mode?1:2;
        uint32_t header[]={0xdadde4,0,0,levels,levels,mipmap_mode,components==4?0u:1u,components,components==4?1u:0u,0,uint32_t(bytes.size()),0,raw_size};
        PixLevel descriptors[]={{2,2,int16_t(codec),0,uint32_t(bytes.size()),raw_size},{1,1,0,0,components,components}};
        const char* path="pix-original-comparison.pix";
        {FILE* file=std::fopen(path,"wb");if(!file)throw std::runtime_error("Cannot write PIX fixture");
            std::fwrite(header,sizeof(header),1,file);std::fwrite(descriptors,sizeof(PixLevel),levels,file);
            std::fwrite(bytes.data(),bytes.size(),1,file);if(levels>1)std::fwrite(bytes.data(),components,1,file);std::fclose(file);}
        using Load=bool(__cdecl*)(const char*,megusta::pix::TextureInfo*,int);
        megusta::pix::TextureInfo old_info{},new_info{};
        uploads.clear();bool old_result=at<Load>(0x462a40)(path,&old_info,all_levels);auto old_uploads=uploads;
        uploads.clear();bool new_result=megusta::pix::load_file(path,&new_info,all_levels);
        compare(old_result==new_result && new_result,0x462a40,"PIX successful decode");
        compare(old_uploads==uploads,0x462ab0,"PIX raw/backreference/unsupported and filtered mip uploads");
        compare(!std::memcmp(&old_info,&new_info,sizeof(old_info)),0x462ab0,"six PIX info fields");
    }
}
}
namespace txf_oracle {
static void APIENTRY gen(GLsizei n,GLuint *p){for(int i=0;i<n;++i)p[i]=17;}
static void APIENTRY bind(GLenum,GLuint){}
static void APIENTRY pixel(GLenum,GLint){}
static void APIENTRY param(GLenum,GLenum,GLint){}
static void APIENTRY release(GLsizei,const GLuint *){}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE own=nullptr;GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(&run),&own);
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
#define HOOK(n,f) state_table_oracle::imports(module,n,reinterpret_cast<void*>(&f),patches)
        HOOK("glTexImage2D",image_oracle::upload);HOOK("glGetIntegerv",image_oracle::integer);
        HOOK("glGenTextures",gen);HOOK("glBindTexture",bind);HOOK("glPixelStorei",pixel);
        HOOK("glTexParameteri",param);HOOK("glTexEnvi",param);HOOK("glDeleteTextures",release);
#undef HOOK
    }
    v0_oracle::Services services;
    using Load=int(__attribute__((thiscall)) *)(void*,const char*,unsigned,unsigned);
    using Glyph=int(__attribute__((thiscall)) *)(void*,unsigned,float*,float*,float*,float*,float*,float*,float*,float*,float*);
    for(unsigned format:{0u,1u})for(unsigned filter:{0u,1u,2u}){
        std::vector<unsigned char> file={255,'t','x','f'};
        auto word=[&](uint32_t x){for(int i=0;i<4;++i)file.push_back((x>>(8*i))&255);};
        word(0x12345678);word(format);word(4);word(4);word(8);word(0);word(1);
        unsigned char glyph[]={32,0,2,3,255,1,4,0,1,0,0,0};file.insert(file.end(),glyph,glyph+12);
        if(format==0)for(unsigned i=0;i<16;++i)file.push_back(i*17);
        else for(unsigned i=0;i<4;++i)file.push_back(i*5);
        const char *path="txf-original-comparison.txf";
        {FILE *f=std::fopen(path,"wb");if(!f)throw std::runtime_error("Cannot write TXF fixture");std::fwrite(file.data(),1,file.size(),f);std::fclose(f);}
        alignas(4) std::array<unsigned char,10264> raw{};uint32_t vtable=0x4ea4b0;std::memcpy(raw.data(),&vtable,4);
        fntTexFont native;double gamma=1.45;at<void(__cdecl *)(double)>(0x461990)(gamma);pngSetViewingGamma(gamma);
        unsigned old_filter=filter==0?0x4f:filter==1?0x50:0x51;
        GLenum native_filter=filter==0?GL_NEAREST:filter==1?GL_LINEAR:GL_NEAREST_MIPMAP_NEAREST;
        image_oracle::uploads.clear();int original=at<Load>(0x48f2c0)(raw.data(),path,0x50,old_filter);auto uploads=image_oracle::uploads;
        image_oracle::uploads.clear();int rebuilt=native.load(path,GL_LINEAR,native_filter);
        compare(original==rebuilt && uploads==image_oracle::uploads,0x48f2c0,"TXF pixel uploads and mip levels");
        std::array<float,9> a{},b{};
        int ga=at<Glyph>(0x45bda0)(raw.data(),32,&a[0],&a[1],&a[2],&a[3],&a[4],&a[5],&a[6],&a[7],&a[8]);
        int gb=native.getGlyph(' ',&b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8]);
        compare(ga==gb && !std::memcmp(a.data(),b.data(),sizeof(a)),0x48f2c0,"TXF space glyph and nine float fields");
    }
    pngSetViewingGamma(1.0);
}
}
namespace embedded_asx_oracle {
struct Provider: ssgLoaderOptions {
    bool found=false,missing=false;std::vector<unsigned char> upload;
    ssgSimpleState *createSimpleState(char*,int) const override {
        auto *self=const_cast<Provider*>(this);ssgTextureInfo info{};
        image_oracle::uploads.clear();self->found=_ssgEmbeddedPixLoader("sample.PIX",&info);
        self->upload=image_oracle::uploads;self->missing=!_ssgEmbeddedPixLoader("absent.pix",&info);
        return new ssgSimpleState;
    }
};
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;HMODULE own=nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(&run),&own);
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
        state_table_oracle::imports(module,"glTexImage2D",reinterpret_cast<void*>(&image_oracle::upload),patches);
        state_table_oracle::imports(module,"glGetIntegerv",reinterpret_cast<void*>(&image_oracle::integer),patches);
    }
    uint32_t depth=32;v0_oracle::MemoryPatch rendering_depth(0x524620,&depth,4);
    v0_oracle::Services services;services.cell(0x52466c,&std::fseek);
    std::vector<unsigned char> file;
    auto word=[&](uint32_t x){for(int i=0;i<4;++i)file.push_back((x>>(8*i))&255);};
    auto setword=[&](size_t p,uint32_t x){for(int i=0;i<4;++i)file[p+i]=(x>>(8*i))&255;};
    word(0x1234abd1);word(0x78620);word(0);word(0);word(1);word(1);word(2);
    word(0x20304050);for(int i=0;i<23;++i)word(i==0?4:i==18?12:0);
    const unsigned char strings[]={ 'm','a','t',0,'s','a','m','p','l','e','.','p','i','x',0,0 };
    file.insert(file.end(),strings,strings+sizeof(strings));
    word(0x60708090);for(int i=0;i<272;++i)word(i==271?1:0);
    word(1);word(3);word(0);word(0);for(int i=0;i<4;++i)word(0);
    word(0);word(1);word(2);for(int i=0;i<7;++i)word(0);
    for(uint32_t bits:{0u,0u,0u,0x3f800000u,0u,0u,0u,0x3f800000u,0u})word(bits);
    word(0x76543210);uint32_t end=file.size();setword(8,end);setword(12,end);
    uint32_t pix[]={0xdadde4,0,0,1,1,0,1,3,0,0,12,0,12};
    struct Level {int16_t w,h,codec,padding;uint32_t packed,unpacked;};Level level{2,2,0,0,12,12};
    for(const char *name:{"other","sample"}){
        word(0xfeedface);word(0);word(sizeof(pix)+sizeof(level)+12);
        char record[16]={};std::strcpy(record,name);file.insert(file.end(),record,record+16);
        auto append=[&](const void *p,size_t n){auto *b=static_cast<const unsigned char*>(p);file.insert(file.end(),b,b+n);};
        append(pix,sizeof(pix));append(&level,sizeof(level));for(int i=0;i<12;++i)file.push_back(i*11);
    }
    const char *path="embedded-original-comparison.asx";
    {FILE *f=std::fopen(path,"wb");if(!f)throw std::runtime_error("Cannot write ASX fixture");std::fwrite(file.data(),1,file.size(),f);std::fclose(f);}
    FILE *source=std::fopen(path,"rb");if(!source)throw std::runtime_error("Cannot open ASX fixture");
    uint32_t ptr=reinterpret_cast<uintptr_t>(source),count=2;
    v0_oracle::MemoryPatch pfile(0x54a060,&ptr,4),pcount(0x54a064,&count,4),poffset(0x54a068,&end,4);
    using Lookup=int(__cdecl *)(const char*);
    image_oracle::uploads.clear();int found=at<Lookup>(0x48b340)("sample.PIX");auto expected=image_oracle::uploads;
    int missing=at<Lookup>(0x48b340)("absent.pix");std::fclose(source);
    Provider provider;auto *prior=ssgGetCurrentOptions();ssgSetCurrentOptions(&provider);
    auto scene=megusta::asx::load_asx_scene(path);ssgSetCurrentOptions(prior);
    compare(found && !missing && provider.found && provider.missing && expected==provider.upload,
            0x48b340,"case-insensitive embedded record lookup, miss and PIX uploads");
    compare(_ssgEmbeddedPixLoader==nullptr,0x48b340,"embedded loader scope restored");
}
}
namespace compiled_leaf_oracle {
static GLuint called;
static void APIENTRY call(GLuint list){called=list;}
static void APIENTRY release(GLuint,GLsizei){}
class Source : public ssgVtxTable {
public:
    Source(){cached_vertices_count=12;gltype=GL_TRIANGLES;dlist=37;
        sgVec3 center={1,2,3};bsphere.setCenter(center);bsphere.setRadius(5);bsphere_is_invalid=FALSE;}
};
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE own=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&run),&own))throw std::runtime_error("Cannot locate compiled-leaf oracle");
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
        state_table_oracle::imports(module,"glCallList",reinterpret_cast<void*>(&call),patches);
        state_table_oracle::imports(module,"glDeleteLists",reinterpret_cast<void*>(&release),patches);
    }
    v0_oracle::Services services;
    v0_oracle::Raw source(0x4ebf7c,0x4007,"source"),target(0,0,"target");
    source.set(80,1);source.set(84,2);source.set(92,12);source.set(116,37);source.set(144,8);
    float sphere[]={1,2,3,5};source.floats(60,sphere,4);
    using Construct=void*(__attribute__((thiscall)) *)(void*,void*);
    at<Construct>(0x47e1f0)(target.pointer(),source.pointer());
    Source native_source;native_source.setRenderOrderMode(2);ssgCompiledLeaf native(&native_source);
    uint32_t old_list,old_source_list;std::memcpy(&old_list,target.bytes.data()+116,4);std::memcpy(&old_source_list,source.bytes.data()+116,4);
    compare(old_list==native.getDListIndex() && old_source_list==native_source.getDListIndex(),0x47e1f0,"move display-list ownership");
    compare(native.getRenderOrderMode()==2 && native.getNumVertices()==0 && native.getNumTriangles()==0,0x47e1f0,"leaf counts stay zero, compiled counters separate");
    compare(!std::memcmp(target.bytes.data()+60,native.getBSphere()->getCenter(),12) && native.getBSphere()->getRadius()==5,0x47e1f0,"copied sphere");
    ssgContext* saved=_ssgCurrentContext;
    {
        ssgContext context;BinaryObject rawcontext;rawcontext.set(80,1);
        uint32_t pointer=reinterpret_cast<uintptr_t>(rawcontext.pointer());v0_oracle::MemoryPatch current(0x53db60,&pointer,4);
        uint32_t old_stats[3];std::memcpy(old_stats,reinterpret_cast<void*>(0x53e7cc),12);
        called=0;at<Action>(0x47e310)(target.pointer());GLuint old_called=called;
        int old_vertices=stats_num_vertices,old_triangles=stats_num_triangles,old_leaves=stats_num_leaves;
        called=0;native.draw();
        compare(called==37 && called==old_called,0x47e310,"compiled display-list draw");
        auto* changed=reinterpret_cast<uint32_t*>(0x53e7cc);
        compare(changed[0]-old_stats[0]==unsigned(stats_num_vertices-old_vertices) &&
                changed[1]-old_stats[1]==unsigned(stats_num_triangles-old_triangles) &&
                changed[2]-old_stats[2]==unsigned(stats_num_leaves-old_leaves),0x47e310,"compiled vertex/triangle/leaf statistics");
    }
    _ssgCurrentContext=saved;
    at<Set>(0x47e2c0)(target.pointer(),0); // Destroy original subobjects without freeing raw fixture storage.
}
}
namespace ase_support_oracle {
static void run(){
    struct MaterialUv {uint32_t prefix[19]{};float repeat[2],offset[2];};
    using Uv=void(__cdecl*)(void*,float*);
    for(int i=0;i<256;++i){
        MaterialUv material;
        material.repeat[0]=float(i-128)*0.13271f;material.repeat[1]=float(i+1)*0.010131f;
        material.offset[0]=float(i%13)*0.173f;material.offset[1]=float(i%7)*-0.51f;
        float old[]={float(float(i-37)*0.113f),float(float(i+1)*0.00000317f)},now[]={old[0],old[1]};
        at<Uv>(0x464f20)(&material,old);_ssgAseTexCoord(now,material.repeat,material.offset);
        compare(!std::memcmp(old,now,sizeof(old)),0x464f20,"retained UV flip and rounded multiply/add stores");
    }
    v0_oracle::Services services;
    using Build=void*(__cdecl*)(void*);
    const float values[]={-1,-0.01f,0,0.01f,0.010000001f,0.5f,1};
    bool saved=_ssgUseMaterialTransparency;
    for(uint32_t enabled=0;enabled<2;++enabled){
        v0_oracle::MemoryPatch flag(0x50781c,&enabled,4);_ssgUseMaterialTransparency=enabled;
        for(float transparency:values){
            alignas(4) std::array<unsigned char,92> material{};
            std::memcpy(material.data()+68,&transparency,4);
            void* original=at<Build>(0x464dd0)(material.data());
            std::unique_ptr<ssgSimpleState> native(_ssgBuildAseMaterialState(nullptr,17,transparency));
            uint32_t old_enables,old_care,old_shade;
            std::memcpy(&old_care,static_cast<unsigned char*>(original)+36,4);
            std::memcpy(&old_enables,static_cast<unsigned char*>(original)+40,4);
            std::memcpy(&old_shade,static_cast<unsigned char*>(original)+120,4);
            compare((old_enables & ~old_care & 63)==unsigned(native->enables & ~native->dont_care & 63) &&
                    old_care==unsigned(native->dont_care),0x464dd0,"default ASE state capabilities and care bits");
            compare(at<Get>(0x46d110)(original)==unsigned(native->isTranslucent()) && old_shade==0x3d &&
                    native->shade_model==GL_SMOOTH,0x464dd0,"ASE transparency threshold/enable flag and smooth shade");
            at<Set>(0x46c260)(original,1);
        }
    }
    _ssgUseMaterialTransparency=saved;
}
}
namespace context_fov_oracle {
static void run(){
    ssgContext* saved=_ssgCurrentContext;
    {
        ssgContext native;
        using SetFov=void(__attribute__((thiscall)) *)(void*,float,float);
        const float values[][2]={{0,60},{90,0},{-1,45},{45,-1},{40,30},{60,45}};
        for(auto& value:values){
            std::array<float,36> original{};original[4]=1;original[5]=10000;
            BinaryObject raw;raw.set(8,reinterpret_cast<uintptr_t>(original.data()));raw.set(12,1);
            at<SetFov>(0x4599b0)(raw.pointer(),value[0],value[1]);native.setFOV(value[0],value[1]);
            float w,h;native.getFOV(&w,&h);
            compare(w==original[34] && h==original[35] && raw.get(12)==0,0x4599b0,"context3:2 FOV fallback");
            sgMat4 matrix;native.getProjectionMatrix(matrix);
            compare(!std::memcmp(matrix,original.data()+18,sizeof(matrix)),0x4599b0,"context FOV projection bits");
        }
    }
    _ssgCurrentContext=saved;
}
}
namespace parser_oracle {
static void run(){
    v0_oracle::Services services;services.cell(0x524654,&std::fgets);
    const char* path="parser-original-comparison.ase";
    {FILE* file=std::fopen(path,"wb");if(!file)throw std::runtime_error("Cannot write parser fixture");
        std::fputs("// empty\r\n*ROOT {\r\n  *ITEM \"hello world\" 17 -2.25 : , // tail\r\n}\r\n",file);std::fclose(file);}
    _ssgParserSpec spec={"\r\n\t ",":,","{","}",'"',0,"//"};
    using Open=void(__attribute__((thiscall)) *)(void*,const char*,const _ssgParserSpec*);
    using Line=char*(__attribute__((thiscall)) *)(void*,int);
    using Token=char*(__attribute__((thiscall)) *)(void*,const char*);
    using Integer=int(__attribute__((thiscall)) *)(void*,const char*);
    using Number=double(__attribute__((thiscall)) *)(void*,const char*);
    std::array<unsigned char,0x3630> raw{};_ssgParser native;
    at<Open>(0x4b5230)(raw.data(),path,&spec);native.openFile(path,&spec);
    compare(native.isOpen(),0x4b5230,"text-mode parser open");
    for(unsigned line=0;line<3;++line){
        char* old=at<Line>(0x4b5410)(raw.data(),0);char* now=native.getLine();
        compare(old && now && !std::strcmp(old,now),0x4b5410,"line comments, braces and CRLF");
        if(line==1){
            old=at<Token>(0x4b5770)(raw.data(),"string");now=native.parseString("string");
            compare(old && now && !std::strcmp(old,now),0x4b5770,"unquoted string result");
            compare(at<Integer>(0x4b5810)(raw.data(),"integer")==native.parseInt("integer"),0x4b5810,"atoi token");
            compare(at<Number>(0x4b57f0)(raw.data(),"number")==native.parseFloat("number"),0x4b57f0,"atof double token");
        }
        do{
            old=at<Token>(0x4b5720)(raw.data(),nullptr);now=native.parseToken(nullptr);
            compare((!old && !now) || (old && now && !std::strcmp(old,now)),0x4b5720,"one-char tokens and null line end");
        }while(old && now);
    }
    compare(!at<Line>(0x4b5410)(raw.data(),0) && !native.getLine(),0x4b5410,"EOF null result");
    at<Action>(0x4b5350)(raw.data());native.closeFile();
    compare(!native.isOpen(),0x4b5350,"closed file state");
}
}
namespace animated_transform_oracle {
class Child:public ssgBranch {public:Child(){sgVec3 c={1,0,0};bsphere.setCenter(c);bsphere.setRadius(2);bsphere_is_invalid=FALSE;}};
class Probe:public megusta::asx::GameAnimatedTransform {
public:using GameAnimatedTransform::GameAnimatedTransform;
    bool dirty()const{return bsphere_is_invalid!=0;}
};
static void run(){
    sgMat4 matrices[3];for(auto& matrix:matrices)sgMakeIdentMat4(matrix);
    matrices[1][3][0]=-5;matrices[2][0][0]=2;matrices[2][3][0]=5;
    std::vector<megusta::asx::HostMatrix> frames;
    for(auto& matrix:matrices)frames.push_back(megusta::asx::host_matrix(matrix));
    Probe native(std::move(frames));native.addKid(new Child);
    v0_oracle::Raw array(0,1,"frames"),raw(0x4ebe44,0x1000073,"animated"),child(0,0x13,"child");
    v0_oracle::array(array,0,64,3,matrices);
    float sphere[]={1,0,0,2};child.floats(60,sphere,4);raw.add(child);
    raw.set(236,reinterpret_cast<uintptr_t>(array.pointer()));raw.set(240,3);raw.set(232,1);
    sgMat4 identity;sgMakeIdentMat4(identity);raw.floats(100,&identity[0][0],16);raw.floats(164,&identity[0][0],16);
    at<Action>(0x47de10)(raw.pointer());auto* native_sphere=native.getBSphere();
    compare(!std::memcmp(raw.bytes.data()+60,native_sphere->getCenter(),12) && native_sphere->getRadius()==7.5f,
            0x47de10,"all-frame sphere union without scaling radius");
    int saved=ssgGetFrameCounter();uint32_t clock=11;v0_oracle::MemoryPatch original_clock(0x53dde8,&clock,4);ssgSetFrameCounter(11);
    for(int frame:{-3,0,1,2,9}){
        at<Set>(0x47dc30)(raw.pointer(),frame);native.setFrame(frame);
        sgMat4 now,previous;native.getTransform(now);native.getLastTransform(previous);
        uint32_t selected,dirty;std::memcpy(&selected,raw.bytes.data()+244,4);std::memcpy(&dirty,raw.bytes.data()+76,4);
        compare(selected==unsigned(native.frame()) && !std::memcmp(raw.bytes.data()+100,now,sizeof(now)),0x47dc30,"signed frame clamp and selected matrix");
        compare(!std::memcmp(raw.bytes.data()+164,previous,sizeof(previous)),0x47dc30,"frame transform history");
        compare(!dirty && !native.dirty(),0x47dc30,"frame changes retain cached all-frame bounds");
    }
    ssgSetFrameCounter(saved);
}
}
#ifdef MEGUSTA_ORACLE_PUI
namespace cursor_oracle {
static std::vector<uint32_t> events;
static DWORD ticks;
static uint32_t bits(float x){uint32_t result;std::memcpy(&result,&x,4);return result;}
static void event(unsigned id,std::initializer_list<uint32_t> values){events.push_back(id);events.insert(events.end(),values.begin(),values.end());}
static DWORD WINAPI clock(){return ticks;}
static void APIENTRY enable(GLenum x){event(1,{x});}
static void APIENTRY disable(GLenum x){event(2,{x});}
static void APIENTRY begin(GLenum x){event(3,{x});}
static void APIENTRY end(){event(4,{});}
static void APIENTRY alpha(GLenum x,GLfloat y){event(5,{x,bits(y)});}
static void APIENTRY blend(GLenum x,GLenum y){event(6,{x,y});}
static void APIENTRY colour(GLfloat x,GLfloat y,GLfloat z,GLfloat w){event(7,{bits(x),bits(y),bits(z),bits(w)});}
static void APIENTRY colourv(const GLfloat *p){colour(p[0],p[1],p[2],p[3]);}
static void APIENTRY bind(GLenum x,GLuint y){event(8,{x,y});}
static void APIENTRY uv(GLfloat x,GLfloat y){event(9,{bits(x),bits(y)});}
static void APIENTRY vertex(GLfloat x,GLfloat y,GLfloat z){event(10,{bits(x),bits(y),bits(z)});}
static void APIENTRY vertexi(GLint x,GLint y){event(11,{uint32_t(x),uint32_t(y)});}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;
    HMODULE dll=nullptr;GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(&run),&dll);
    for(HMODULE module:{GetModuleHandleA(nullptr),dll}){
#define PATCH(n,f) state_table_oracle::imports(module,n,reinterpret_cast<void*>(&f),patches)
        PATCH("glEnable",enable);PATCH("glDisable",disable);PATCH("glBegin",begin);PATCH("glEnd",end);
        PATCH("glAlphaFunc",alpha);PATCH("glBlendFunc",blend);PATCH("glColor4f",colour);PATCH("glColor4fv",colourv);
        PATCH("glBindTexture",bind);PATCH("glTexCoord2f",uv);PATCH("glVertex3f",vertex);PATCH("glVertex2i",vertexi);
        PATCH("timeGetTime",clock);
#undef PATCH
    }
    using Configure=void(__cdecl *)(unsigned,int,int,int,int,bool);
    using Draw=void(__cdecl *)(int,int);
    for(unsigned texture:{0u,7u})for(bool fade:{false,true}){
        at<Configure>(0x456420)(texture,-2,3,17,19,fade);puSetCursorTexture(texture,-2,3,17,19,fade);
        ticks=1000;at<Draw>(0x456380)(20,30);puCursor(20,30);
        for(DWORD now:{1000u,7000u,8000u,10000u,11000u,0x80000000u}){
            ticks=now;events.clear();at<Draw>(0x456670)(20,100);auto original=events;
            events.clear();puDrawCursor(20,100);
            compare(original==events,0x456670,"complete cursor GL stream and idle fade bits");
        }
    }
    puSetCursorTexture(0,0,0,0,0,false);
}
}
namespace input_key_oracle {
static void run(){
    using Key=int(__attribute__((thiscall)) *)(void*,int,int);
    uint32_t zero=0;v0_oracle::MemoryPatch active(0x53da90,&zero,4);
    for(bool bound:{false,true})for(int key:{8,127,0x15,int('Z'),PU_KEY_LEFT,PU_KEY_RIGHT,PU_KEY_HOME,PU_KEY_END,3,PU_KEY_UP}){
        alignas(4) std::array<unsigned char,452> raw{};
        auto set=[&](unsigned offset,uint32_t value){std::memcpy(raw.data()+offset,&value,4);};
        auto get=[&](unsigned offset){uint32_t value;std::memcpy(&value,raw.data()+offset,4);return value;};
        char original_bound[80]="abcd",native_bound[80]="abcd";
        std::strcpy(reinterpret_cast<char*>(raw.data()+16),"abcd");
        set(340,1);set(344,1);set(432,1);set(436,2);set(440,1);set(444,3);
        if(bound)set(104,reinterpret_cast<uintptr_t>(original_bound));
        puInput native(0,0,20,20);native.setValue("abcd");
        if(bound)native.setValuator(native_bound);
        native.acceptInput();native.setCursor(2);native.setSelectRegion(1,3);puDeactivateWidget();
        int a=at<Key>(0x46a3e0)(raw.data(),key,0);int b=native.checkKey(key,0);
        int start,end;native.getSelectRegion(&start,&end);
        const char *old=bound?original_bound:reinterpret_cast<char*>(raw.data()+16);
        if(a!=b || std::strcmp(old,native.getStringValue()) || get(436)!=uint32_t(native.getCursor()) ||
           get(440)!=uint32_t(start) || get(444)!=uint32_t(end) || get(432)!=uint32_t(native.isAcceptingInput()))
            std::fprintf(stderr,"input mismatch bound=%d key=%d result=%d/%d text=%s/%s cursor=%u/%d selection=%u,%u/%d,%d accepting=%u/%d\n",bound,key,a,b,old,native.getStringValue(),get(436),native.getCursor(),get(440),get(444),start,end,get(432),native.isAcceptingInput());
        compare(a==b && !std::strcmp(old,native.getStringValue()) &&
                get(436)==uint32_t(native.getCursor()) && get(440)==uint32_t(start) &&
                get(444)==uint32_t(end) && get(432)==uint32_t(native.isAcceptingInput()),
                0x46a3e0,"input key, selection and optional string binding");
    }
}
}
namespace pui_box_oracle {
static void APIENTRY shade(GLenum mode){cursor_oracle::event(12,{mode});}
static void APIENTRY rect(GLint x1,GLint y1,GLint x2,GLint y2){
    // OpenGL's glRecti is equivalent to the four-vertex polygon used by the game.
    cursor_oracle::begin(GL_POLYGON);cursor_oracle::vertexi(x1,y1);cursor_oracle::vertexi(x2,y1);
    cursor_oracle::vertexi(x2,y2);cursor_oracle::vertexi(x1,y2);cursor_oracle::end();
}
static void run(){
    std::vector<std::unique_ptr<v0_oracle::MemoryPatch>> patches;HMODULE own=nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(&run),&own);
    for(HMODULE module:{GetModuleHandleA(nullptr),own}){
#define HOOK(n,f) state_table_oracle::imports(module,n,reinterpret_cast<void*>(&f),patches)
        HOOK("glBegin",cursor_oracle::begin);HOOK("glEnd",cursor_oracle::end);
        HOOK("glVertex2i",cursor_oracle::vertexi);HOOK("glColor4f",cursor_oracle::colour);
        HOOK("glColor4fv",cursor_oracle::colourv);HOOK("glShadeModel",shade);
        if(module==own)HOOK("glRecti",rect);
#undef HOOK
    }
    using Draw=void(__attribute__((thiscall)) *)(void*,int,int,int,float (*)[4],int);
    int bounds[]={3,7,64,40};puBox box{{3,7},{64,40}};
    puColour colors[5]={{0.2f,0.4f,0.6f,0.8f},{0.1f,0.3f,0.5f,0.7f},
        {0.8f,0.7f,0.6f,0.9f},{0.3f,0.2f,0.1f,0.4f},{0.9f,0.6f,0.4f,0.2f}};
    for(int style=-9;style<=9;++style)for(int selected:{0,1}){
        cursor_oracle::events.clear();at<Draw>(0x4657d0)(bounds,2,-3,style,colors,selected);auto original=cursor_oracle::events;
        cursor_oracle::events.clear();box.draw(2,-3,style,colors,selected);
        bool same=original==cursor_oracle::events;
        if(!same)std::fprintf(stderr,"box style=%d default=%d events=%u/%u\n",style,selected,unsigned(original.size()),unsigned(cursor_oracle::events.size()));
        compare(same,0x4657d0,"PUI box ordered GL output");
    }
}
}
namespace slider_oracle {
static void run(){
    using Hit=void(__attribute__((thiscall)) *)(void*,int,int,int,int);
    // The suspended game's VC6 CRT startup has not run. Its float formatter
    // otherwise reports "floating point not loaded" and blocks the oracle.
    // Redirect only this service boundary; the original slider body executes.
    v0_oracle::Services services;services.jump(0x4da260,&std::sprintf);
    uint32_t zero=0;v0_oracle::MemoryPatch active(0x53da90,&zero,4);
    for(int vertical:{0,1})for(int mode:{0,1,2})for(int edge:{0,1})for(int position:{-10,0,15,50,100,120}){
        alignas(4) std::array<unsigned char,452> raw{};
        auto set=[&](unsigned off,uint32_t v){std::memcpy(raw.data()+off,&v,4);};
        auto get=[&](unsigned off){uint32_t v;std::memcpy(&v,raw.data()+off,4);return v;};
        float fraction=0.1f,delta=0.1f;
        set(232,0);set(236,0);set(240,vertical?10:100);set(244,vertical?100:10);
        set(332,1);set(432,vertical);std::memcpy(raw.data()+440,&delta,4);set(444,mode);std::memcpy(raw.data()+448,&fraction,4);
        puSlider native(0,0,100,vertical,10);native.setCBMode(mode);native.setDelta(delta);native.setSliderFraction(fraction);
        puDeactivateWidget();*reinterpret_cast<uint32_t*>(0x53da90)=0;
        at<Hit>(0x466ca0)(raw.data(),0,edge,vertical?0:position,vertical?position:0);
        bool original_active=*reinterpret_cast<uint32_t*>(0x53da90)==reinterpret_cast<uintptr_t>(raw.data());
        native.doHit(0,edge,vertical?0:position,vertical?position:0);float value;native.getValue(&value);
        uint32_t bits;std::memcpy(&bits,&value,4);
        compare(get(12)==bits && original_active==(puActiveWidget()==&native),0x466ca0,"slider float and callback-mode active state");
        puDeactivateWidget();
    }
}
}
#endif
namespace ase_writer_oracle {
}
// Internal writer overload; public PLIB header only exposes filename service.
extern int ssgSaveASE(FILE *,ssgEntity *);
namespace ase_writer_oracle {
static void run(){
    v0_oracle::Services services;
    services.cell(0x52465c,&std::fprintf);services.cell(0x52464c,&std::fflush);
    using Save=int(__cdecl *)(FILE*,void*);
    for(int variant=0;variant<2;++variant){
        v0_oracle::Raw root(0x4ebeac,0x113,"root"),leaf(0x4ebf7c,0x4007,"mesh"),vertices(0,1,"");
        ssgSelector native(2);
        sgVec3 points[3]={{0,0,0},{1,0,0},{0,1,0}};
        if(variant){
            v0_oracle::array(vertices,0x4eb01c,12,3,points);
            leaf.set(92,3);leaf.set(144,8);leaf.set(148,reinterpret_cast<uintptr_t>(vertices.pointer()));
            auto *array=new ssgVertexArray;for(auto& point:points)array->add(point);
            auto *mesh=new ssgVtxTable(GL_TRIANGLES,array,nullptr,nullptr,nullptr);mesh->setName("mesh");
            native.addKid(mesh);native.addKid(mesh);root.add(leaf);root.add(leaf);
        }
        // Fixed names stay in the staged C: working directory; avoid CRT
        // tmpfile services that consult host-derived temporary paths in Wine.
        FILE *original=std::fopen("ase-original-output.txt","w+b"),
             *rebuilt=std::fopen("ase-native-output.txt","w+b");
        if(!original || !rebuilt)throw std::runtime_error("Cannot create ASE writer comparison streams");
        std::fprintf(stderr,"ASE writer fixture%d original\n",variant);
        int a=at<Save>(0x48ba50)(original,root.pointer());
        std::fprintf(stderr,"ASE writer fixture%d native\n",variant);
        int b=ssgSaveASE(rebuilt,&native);
        compare(a==b && v0_oracle::stream_bytes(original)==v0_oracle::stream_bytes(rebuilt),0x48ba50,"complete ASE output including all selector children");
        std::fclose(original);std::fclose(rebuilt);
    }
}
}
namespace game_animation_oracle {
static void run(){
    using Count=unsigned(__cdecl *)(void*);
    using Apply=void(__attribute__((thiscall)) *)(void*,void*);
    using Callback=int(__cdecl *)(void*,int);
    v0_oracle::Raw root(0x4eb77c,0x13,"root"),selector(0x4ebeac,0x113,"selector"),
        leaf(0x4ebf7c,0x4007,"leaf"),state(0x4ec2e0,0x1d,"state"),
        a(0x4eb77c,0x13,"a"),b(0x4eb77c,0x13,"b");
    root.add(selector);root.add(leaf);selector.add(a);selector.add(b);
    unsigned char selected[2]={0,0};selector.set(100,reinterpret_cast<uintptr_t>(selected));selector.set(104,2);
    leaf.set(88,reinterpret_cast<uintptr_t>(state.pointer()));state.set(132,3);
    ssgBranch native;auto *choices=new ssgSelector(2);choices->addKid(new ssgBranch);choices->addKid(new ssgBranch);
    native.addKid(choices);auto *mesh=new ssgVtxTable;auto *states=new ssgStateSelector(3);mesh->setState(states);native.addKid(mesh);
    compare(at<Count>(0x46d4c0)(root.pointer())==ssgAnim::countFrames(&native),0x46d4c0,"recursive selector and material frame count");
    BinaryObject animation;animation.set(28,reinterpret_cast<uintptr_t>(root.pointer()));animation.set(40,1);
    auto *controller=new ssgAnim(&native);
    for(unsigned frame:{0u,1u,2u,7u,0xffffffffu}){
        at<Set>(0x46d6e0)(animation.pointer(),frame);controller->setFrame(frame);
        compare(controller->getFrame()==frame,0x46d6e0,"deferred frame stored without clamping");
        at<Apply>(0x46d700)(animation.pointer(),root.pointer());
        native.getTravCallback(SSG_CALLBACK_PRETRAV)(&native,1);
        uint32_t oldstep;std::memcpy(&oldstep,state.bytes.data()+136,4);
        compare(choices->getSelect()==unsigned(selected[0]|(selected[1]<<1)) && states->getSelectStep()==oldstep,
                0x46d700,"recursive selector/material frame clamping");
    }
    // Original traversal callback is an interior entry, not a separate inventory row.
    root.set(24,reinterpret_cast<uintptr_t>(animation.pointer()));
    compare(at<Callback>(0x46d7f0)(root.pointer(),0)==1,0x46d700,"non-cull traversal leaves frame selection alone");
}
}
extern "C" __declspec(dllexport) DWORD WINAPI MegustaRunOriginalChecks(void* result_path) {
    FILE* result=std::fopen(static_cast<const char*>(result_path),"w");
    if(!result)return 2;
    try {
        static_assert(sizeof(void*)==4,"Original oracle is x86 only");
        OriginalImage original;
        uint16_t control;asm volatile("fnstcw %0":"=m"(control));
        state_checks();retained_dot_checks();gamma_checks();capability_checks();selector_checks();math_checks();geometry_checks();frustum_checks();
#ifdef MEGUSTA_ORACLE_NET
        record_checks();
        net_error_checks();
#endif
#ifdef MEGUSTA_ORACLE_PUI
        font_checks();
        palette_checks();
#endif
        unsigned v0_start=checks;v0_oracle::run();unsigned v0_checks=checks-v0_start;
        io_oracle::run();
        state_table_oracle::run();
        cutout_oracle::run();
        material_oracle::run();
        image_oracle::run();
        txf_oracle::run();
        embedded_asx_oracle::run();
        compiled_leaf_oracle::run();
        ase_support_oracle::run();
        context_fov_oracle::run();
        parser_oracle::run();
        animated_transform_oracle::run();
        game_animation_oracle::run();
        ase_writer_oracle::run();
#ifdef MEGUSTA_ORACLE_PUI
        cursor_oracle::run();
        input_key_oracle::run();
        pui_box_oracle::run();
        slider_oracle::run();
#endif
        if(mismatches)throw std::runtime_error("Original PLIB differential failed; see mismatch log");
        std::fprintf(result,"{\"checks\":%u,\"x87_control_word\":%u,\"functions\":[",checks,control);
        bool first=true;for(uint32_t address:compared_functions){std::fprintf(result,"%s\"%08x\"",first?"":",",address);first=false;}
        std::fprintf(result,"],\"ssg_v0\":{\"version\":0,\"checks\":%u,\"redirected_services\":[\"file\",\"allocation\",\"diagnostic\"],\"texture_image_loading_compared\":false}}\n",v0_checks);
        std::fclose(result);return 0;
    } catch(const std::exception& e){std::fprintf(result,"{\"error\":\"%s\",\"checks\":%u}\n",e.what(),checks);std::fclose(result);return 1;}
}
