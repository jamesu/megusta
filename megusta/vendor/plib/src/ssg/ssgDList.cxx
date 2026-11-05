/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 2001  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id: ssgDList.cxx,v 1.6 2001/04/12 03:38:15 deca Exp $
*/


#include "ssgLocal.h"


// MG-PLIB-026: adapted executable 0048ab20..0048af46 command queue.
// Original opcodes: leaf=0, load=1, push=2, pop=3, consumed=4.
// Matrix/texture command extensions in PLIB 1.3.2 are absent in this build.
enum _ssgDListType { SSG_DLIST_LEAF, SSG_DLIST_LOAD_MATRIX,
  SSG_DLIST_PUSH_MATRIX, SSG_DLIST_POP_MATRIX, SSG_DLIST_NOTHING };
struct _ssgDList {
  _ssgDListType type;
  sgMat4 mat;
  ssgLeaf *leaf;
  unsigned int state_step;
  float sort_depth;
};
static const int MAX_DLIST=512;
static int next_dlist=0;
static _ssgDList dlist[MAX_DLIST];
static _ssgDList *sorted[MAX_DLIST];

void _ssgDrawDList ()
{
  int count=0;
  for(int i=0;i<next_dlist;++i) {
    _ssgDList &command=dlist[i];
    switch(command.type) {
      case SSG_DLIST_LEAF: {
        glGetFloatv(GL_MODELVIEW_MATRIX,(float*)command.mat);
        if(command.leaf->getRenderOrderMode()<2) {
          sgSphere *first=command.leaf->getBSphere();
          sgSphere *second=command.leaf->getBSphere();
          // x87 instruction order is y*m6, z*m10, x*m2, radius, m14;
          // retain intermediates at the selected 53-bit FPU precision.
          const float *center=first->getCenter();
          double depth=double(command.mat[1][2])*center[1];
          depth+=double(command.mat[2][2])*center[2];
          depth+=double(command.mat[0][2])*center[0];
          depth+=second->getRadius();depth+=command.mat[3][2];
          command.sort_depth=float(-depth);
        } else command.sort_depth=1000000.0f;
        break;
      }
      case SSG_DLIST_PUSH_MATRIX:
        glPushMatrix();
        // fall through
      case SSG_DLIST_LOAD_MATRIX:
        glLoadMatrixf((float*)command.mat);command.type=SSG_DLIST_NOTHING;break;
      case SSG_DLIST_POP_MATRIX:
        glPopMatrix();command.type=SSG_DLIST_NOTHING;break;
      default:break;
    }
    if(command.type==SSG_DLIST_LEAF)sorted[count++]=&command;
  }
  next_dlist=0;
  bool finished;
  do {
    finished=true;
    for(int i=1;i<count;++i)if(sorted[i-1]->sort_depth<sorted[i]->sort_depth) {
      _ssgDList *temporary=sorted[i-1];sorted[i-1]=sorted[i];sorted[i]=temporary;finished=false;
    }
  } while(!finished);
  glPushMatrix();
  bool depth_write=true;
  for(int i=0;i<count;++i) {
    _ssgDList &command=*sorted[i];
    bool wanted=command.leaf->getRenderOrderMode()==1;
    if(depth_write!=wanted){glDepthMask(wanted);depth_write=wanted;}
    if(command.type==SSG_DLIST_LEAF) {
      ssgState *state=command.leaf->getState();
      if(state && state->isAKindOf(ssgTypeStateSelector()))
        static_cast<ssgStateSelector*>(state)->selectStep(command.state_step);
      glLoadMatrixf((float*)command.mat);command.leaf->draw();
    }
    command.type=SSG_DLIST_NOTHING;
  }
  if(!depth_write)glDepthMask(GL_TRUE);
  glPopMatrix();
}

void _ssgLoadMatrix ( sgMat4 m )
{
  while(next_dlist>0 && dlist[next_dlist-1].type==SSG_DLIST_LOAD_MATRIX)--next_dlist;
  if(next_dlist>=MAX_DLIST){ulSetError(UL_WARNING,"DList stack overflow!");return;}
  _ssgDList &command=dlist[next_dlist++];sgCopyMat4(command.mat,m);command.type=SSG_DLIST_LOAD_MATRIX;
}
void _ssgPushMatrix ( sgMat4 m )
{
  if(next_dlist>0 && dlist[next_dlist-1].type==SSG_DLIST_POP_MATRIX) {
    --next_dlist;_ssgLoadMatrix(m);return;
  }
  if(next_dlist>=MAX_DLIST){ulSetError(UL_WARNING,"DList stack overflow!");return;}
  _ssgDList &command=dlist[next_dlist++];sgCopyMat4(command.mat,m);command.type=SSG_DLIST_PUSH_MATRIX;
}
void _ssgPopMatrix ()
{
  if(next_dlist>0 && dlist[next_dlist-1].type==SSG_DLIST_PUSH_MATRIX){--next_dlist;return;}
  if(next_dlist>=MAX_DLIST){ulSetError(UL_WARNING,"DList stack overflow!");return;}
  dlist[next_dlist++].type=SSG_DLIST_POP_MATRIX;
}
void _ssgDrawLeaf ( ssgLeaf *leaf )
{
  if(next_dlist>=MAX_DLIST){ulSetError(UL_WARNING,"DList stack overflow!");return;}
  _ssgDList &command=dlist[next_dlist++];command.leaf=leaf;
  ssgState *state=leaf->getState();
  if(state && state->isAKindOf(ssgTypeStateSelector()))
    command.state_step=static_cast<ssgStateSelector*>(state)->getSelectStep();
  command.type=SSG_DLIST_LEAF;
}
