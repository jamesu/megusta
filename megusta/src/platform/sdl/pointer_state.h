// SPDX-License-Identifier: GPL-2.0

#pragma once
namespace megusta::platform {
// Relative SDL motion feeds the original game's recentered absolute cursor.
struct SdlPointerState {
    int x=0,y=0;
    bool valid=false;
    void reset(int px,int py){x=px;y=py;valid=true;}
    void motion(int px,int py,int dx,int dy,bool relative){
        if(relative && valid){x+=dx;y+=dy;}
        else reset(px,py);
    }
};
}
