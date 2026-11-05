#include "bindings/gameplay.h"
#include "game/player.h"
#include "game/balls.h"
#include "fnt.h"
#include <cstdio>
#include <cstring>
namespace {
class ProbeFont final:public fntFont {
public:
    int begins=0,ends=0,characters=0,strings=0;
    void getBBox(const char*,float,float,float* l,float* r,float* b,float* t) override { *l=1; *r=12.75f; *b=0; *t=10; }
    void putch(sgVec3 p,float,float,char) override { ++characters; p[0]+=1; }
    void puts(sgVec3 p,float,float,const char*) override { ++strings; p[0]+=2; }
    void begin() override { ++begins; }
    void end() override { ++ends; }
    int load(const char*,GLenum,GLenum) override { return 0; }
    void setFixedPitch(int) override {}
    int isFixedPitch() const override { return 0; }
    void setWidth(float) override {}
    void setGap(float) override {}
    float getWidth() const override { return 1; }
    float getGap() const override { return 0; }
    int hasGlyph(char) override { return 1; }
};
}
int checkGameplayBindings() {
    int count=0;
    PyMethodDef* tables[]={megusta::bindings::pointFontMethods(),megusta::bindings::playerBallMethods()};
    for (auto* table:tables) {
        for (auto* method=table;method->ml_name;++method) {
            PyObject* args=!std::strcmp(method->ml_name,"generic_isEditing")
                ?Py_BuildValue(const_cast<char*>("(i)"),1):PyTuple_New(0);
            PyObject* result=method->ml_meth(nullptr,args); Py_DECREF(args);
            if (result || !PyErr_Occurred()) { Py_XDECREF(result); return 20; }
            PyErr_Clear(); ++count;
        }
    }
    megusta::Player player{}; std::strcpy(player.name,"probe"); player.strokeCount=3; player.playerIndex=1;
    megusta::BallsManager balls{}; balls.currentBall=&balls.balls[1];
    ssgBranch special[3];
    for (int i=0;i<3;++i) balls.balls[1].specialModel[i]=&special[i];
    megusta::Random random{0x12345678u};
    ProbeFont font; fntRenderer renderer;
    char setup[1024];
    std::sprintf(setup,
        "player = '_%lx_Player_p'\nballs = '_%lx_Balls_p'\nrandom = '_%lx_Rnd_p'\nfont = '_%lx_fntFont_p'\nrenderer = '_%lx_fntRenderer_p'\n",
        reinterpret_cast<unsigned long>(&player),reinterpret_cast<unsigned long>(&balls),
        reinterpret_cast<unsigned long>(&random),reinterpret_cast<unsigned long>(&font),reinterpret_cast<unsigned long>(&renderer));
    if (PyRun_SimpleString(setup)) { PyErr_Print(); return 21; }
    const char* checks=
        "p = gamec.new_Point3F(3, 4, 0)\nq = gamec.new_Point3F(1, 0, 0)\n"
        "assert gamec.Point3F_len(p) == 5.0\nassert gamec.Point3F_dot(p, q) == 3.0\n"
        "assert gamec.Point3F_x_set(q, 2) == 2.0\n"
        "gamec.Point3F_add(p, q)\nassert gamec.Point3F_x_get(p) == 5.0\n"
        "gamec.Point3F_sub(p, q)\ngamec.Point3F_scale(q, 0.5)\nassert gamec.Point3F_x_get(q) == 0.5\n"
        "gamec.Point3F_setPolar(q, 2, 0)\nassert gamec.Point3F_x_get(q) == 2.0\n"
        "gamec.Point3F_normalize(p)\nassert abs(gamec.Point3F_len(p)-1) < 0.00001\n"
        "import strop\ntry:\n gamec.Point3F_dot(p, '_1_Ball_p')\nexcept TypeError, message:\n assert strop.find(str(message), 'argument 2') >= 0\nelse:\n raise RuntimeError, 'second pointer accepted'\n"
        "assert gamec.Rnd_gen(random, 1) == 0\n"
        "assert gamec.Player_getName(player) == 'probe'\n"
        "gamec.Player_giveBonus(player)\nassert gamec.Player_getStroke(player) == 2\n"
        "gamec.Player_incPenalty(player)\nassert gamec.Player_getStrokep(player) == 3\n"
        "gamec.Player_decPenalty(player)\ngamec.Player_decPenalty(player)\nassert gamec.Player_getStrokep(player) == 2\n"
        "gamec.Player_setPoints(player, 23)\nassert gamec.Player_getPoints(player) == 23\n"
        "gamec.Player_setPlayClock(player, 31)\nassert gamec.Player_getPlayClock(player) == 31\n"
        "gamec.Player_setAttributes(player, 0.25, 0.5, 0.75)\n"
        "assert gamec.Player_getControl(player) == 0.25\nassert gamec.Player_getAccuracy(player) == 0.5\nassert gamec.Player_getLuck(player) == 0.75\n"
        "gamec.Player_setLeftHanded(player)\nassert gamec.Player_getHandedness(player) == 1\n"
        "gamec.Player_setRightHanded(player)\nassert gamec.Player_getHandedness(player) == 0\n"
        "gamec.Player_setRobot(player)\nassert gamec.Player_getIsHuman(player) == 0\n"
        "gamec.Player_setHuman(player)\nassert gamec.Player_getIsHuman(player) == 1\n"
        "ball = gamec.Balls_get(balls, 1)\nassert gamec.Balls_getActive(balls) == ball\n"
        "assert gamec.Ball_isValid('NULL') == 0\nassert gamec.Ball_isValid(ball) == 1\n"
        "assert gamec.Balls_b2b_flag_set(balls, -1) == -1\nassert gamec.Balls_b2b_flag_get(balls) == -1\n"
        "gamec.Ball_setLoc(ball, q)\nloc = gamec.Ball_getLoc(ball)\nassert gamec.Point3F_x_get(loc) == 2\n"
        "gamec.Point3F_z_set(loc, 9)\nassert gamec.Point3F_z_get(gamec.Ball_getLoc(ball)) == 9\n"
        "gamec.Ball_setVel(ball, q)\nassert gamec.Point3F_x_get(gamec.Ball_getVel(ball)) == 2\n"
        "gamec.Ball_setColour(ball, 17)\nassert gamec.Ball_getColour(ball) == 17\n"
        "gamec.Ball_setFrozen(ball, 1)\nassert gamec.Ball_isFrozen(ball) == 1\n"
        "gamec.Ball_setFrozen(ball, 0)\nassert gamec.Ball_isFrozen(ball) == 0\n"
        "gamec.Ball_setShadow(ball, 0)\n"
        "gamec.Ball_cloak(ball)\nassert gamec.Ball_isCloaked(ball) == 1\n"
        "gamec.Ball_unCloak(ball)\nassert gamec.Ball_isCloaked(ball) == 0\n"
        "gamec.Ball_setInCup(ball)\nassert gamec.Ball_inCup(ball) == 1\n"
        "gamec.Ball_setPowerUpFlag(ball, 9)\nassert gamec.Ball_isPowerUpFlagSet(ball, 9) == 1\nassert gamec.Ball_isPowerUpFlagSet(ball, 1) == 0\n"
        "gamec.fntRenderer_setFont(renderer, font)\nassert gamec.fntRenderer_getFont(renderer) == font\n"
        "gamec.fntRenderer_setPointSize(renderer, 14)\nassert gamec.fntRenderer_getPointSize(renderer) == 14\n"
        "gamec.fntRenderer_setSlant(renderer, 0.25)\nassert gamec.fntRenderer_getSlant(renderer) == 0.25\n"
        "gamec.fntRenderer_start2f(renderer, 10, 20)\n"
        "gamec.fntRenderer_begin(renderer)\ngamec.fntRenderer_putch(renderer, 'a')\ngamec.fntRenderer_puts(renderer, 'abc')\ngamec.fntRenderer_end(renderer)\n"
        "assert gamec.fntRenderer_getStringWidth(renderer, 'abc') == 11\n"
        "gamec.delete_Point3F(p)\ngamec.delete_Point3F(q)\n";
    if (PyRun_SimpleString(const_cast<char*>(checks))) { PyErr_Print(); return 22; }
    float x,y,z; renderer.getCursor(&x,&y,&z);
    if (font.begins!=1 || font.ends!=1 || font.characters!=1 || font.strings!=1 || x!=13 || y!=20 || z!=0) return 23;
    if (balls.balls[1].position[2]!=9 || !(balls.balls[1].flags&0x40u)) return 24;
    for (auto& model:special) if (model.getTraversalMask()&1) return 25;
    if (random.state!=0x12345678u) return 26; // limit <= 1 must not advance the generator.
    std::printf("point/font/player/ball bindings: %d argument guards, borrowed pointers, assigned returns, field writes and font dispatch passed\n",count);
    return count==100?0:27;
}
