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

     $Id: ssgLeaf.cxx,v 1.15 2001/04/12 03:38:15 deca Exp $
*/


#include "ssgLocal.h"

//~~ T.G. moved hee from header amd ref counting accounted for
 
void ssgLeaf::setState ( ssgState *st )
{
    ssgDeRefDelete ( state ) ;

    state = st ;

    if ( state != NULL )
      state->ref() ;
}

void ssgLeaf::copy_from ( ssgLeaf *src, int clone_flags )
{
  ssgEntity::copy_from ( src, clone_flags ) ;

  cull_face = src -> getCullFace () ;
  render_order_mode=src->render_order_mode; // MG-PLIB-019: original copy order.

  ssgState *s = src -> getState () ;

  //~~ T.G. Deref
  ssgDeRefDelete(state); 

  if ( s != NULL && ( clone_flags & SSG_CLONE_STATE ) )
    state = (ssgState *)( s -> clone ( clone_flags ) ) ;
  else
    state = s ;

   //~~ T.G. increment ref counter 
   if (state != NULL)  
       state->ref(); 
}



ssgLeaf::ssgLeaf (void)
{
  render_order_mode=0; // MG-PLIB-019
  cached_vertices_count=cached_normals_count=cached_colours_count=cached_texcoords_count=0; // MG-PLIB-032

  cull_face = TRUE ;
  state = NULL ;
  type |= SSG_TYPE_LEAF ;

#ifdef _SSG_USE_DLIST
  dlist = 0 ;
#endif

  preDrawCB = NULL ;
  postDrawCB = NULL ;
}

ssgLeaf::~ssgLeaf (void)
{
  //~~ T.G. ssgDeRefDelete checks for null case
  ssgDeRefDelete ( state ) ;

#ifdef _SSG_USE_DLIST
  deleteDList () ;
#endif
}


#ifdef _SSG_USE_DLIST
void ssgLeaf::deleteDList ()
{
  if ( dlist != 0 )
    glDeleteLists ( dlist, 1 ) ;

  dlist = 0 ;
}

void ssgLeaf::makeDList ()
{
  deleteDList () ;  /* Just to be sure */
  dlist = glGenLists ( 1 ) ;
  glNewList ( dlist, GL_COMPILE ) ;
    draw_geometry () ;
  glEndList () ; 
}
#endif // #ifdef _SSG_USE_DLIST


void ssgLeaf::cull ( sgFrustum *f, sgMat4 m, int test_needed )
{
  int cull_result = cull_test ( f, m, test_needed ) ;

  if ( cull_result == SSG_OUTSIDE )
    return ;

  if ( isTranslucent () )
    _ssgDrawLeaf ( this ) ;
  else
    draw () ;
}

#if !defined(MEGUSTA_NO_SSG_COLLISION) // MG-PLIB-088
void ssgLeaf::hot ( sgVec3 s, sgMat4 m, int test_needed )
{
  int hot_result = hot_test ( s, m, test_needed ) ;

  if ( hot_result == SSG_OUTSIDE )
    return ;

  /* Add polygons to hit list! */

  hot_triangles ( s, m, hot_result != SSG_INSIDE ) ;
}
#endif




#if !defined(MEGUSTA_NO_SSG_COLLISION) // MG-PLIB-088
void ssgLeaf::isect ( sgSphere *s, sgMat4 m, int test_needed )
{
  int isect_result = isect_test ( s, m, test_needed ) ;

  if ( isect_result == SSG_OUTSIDE )
    return ;

  /* Add polygons to hit list! */

  isect_triangles ( s, m, isect_result != SSG_INSIDE ) ;
}
#endif



void ssgLeaf::print ( FILE *fd, char *indent, int how_much )
{
  // MG-PLIB-023: original has no verbosity suppression.

  ssgEntity::print ( fd, indent, how_much  ) ;

  if ( getNumParents () != getRef () )
    fprintf ( fd, "****** WARNING: Ref count doesn't equal parent count!\n" ) ;

  if ( state != NULL )
  {
    char in [ 100 ] ;
    sprintf ( in, "%s  ", indent );

    state -> print ( fd, in, how_much ) ;
  }
  else
    fprintf ( fd, "%s  No State assigned to this node\n", indent ) ;
}


int ssgLeaf::preDraw ()
{
  if ( preDrawCB != NULL && ! (*preDrawCB)(this) )
    return FALSE ;

  _ssgCurrentContext->setCullface ( getCullFace() ) ;

  return TRUE ;
}


int ssgLeaf::load ( FILE *fd )
{
  // MG-PLIB-019: adapted original version-0 record layout.
  int marker=0;_ssgReadInt(fd,&marker);_ssgReadInt(fd,&cull_face);_ssgReadInt(fd,&render_order_mode);
  ssgBase* loaded=NULL;int needs_load=0;
  if(!_ssgV0ReadObjectHeader(fd,&loaded,ssgTypeState(),6,&needs_load))return FALSE;
  setState((ssgState*)loaded); // Original installs/refs the state before its virtual load.
  if(needs_load>0 && !loaded->load(fd))return FALSE;
  _ssgReadInt(fd,&marker);
  return ssgEntity::load(fd);
}


int ssgLeaf::save ( FILE *fd )
{
  // MG-PLIB-019: adapted original version-0 record layout.
  _ssgWriteInt(fd,0x38b83);_ssgWriteInt(fd,cull_face);_ssgWriteInt(fd,render_order_mode);
  if(!_ssgSaveObject(fd,getState()))return FALSE;
  _ssgWriteInt(fd,0x23731f);return ssgEntity::save(fd);
}



