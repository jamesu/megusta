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

     $Id: ssgStateSelector.cxx,v 1.17 2001/04/12 03:38:16 deca Exp $
*/


#include "ssgLocal.h"

void ssgStateSelector::copy_from ( ssgStateSelector *src, int clone_flags )
{
  ssgSimpleState::copy_from ( src, clone_flags ) ;

  nstates   = src -> getNumSteps   () ;
  selection = src -> getSelectStep () ;
  statelist = new ssgSimpleState * [ nstates ] () ; // MG-PLIB-019: defined partial-operation cleanup.

  for ( int i = 0 ; i < nstates ; i++ )
  {
    ssgSimpleState *s = src -> getStep ( i ) ;

    if ( s != NULL && (clone_flags & SSG_CLONE_STATE_RECURSIVE) )
      statelist [ i ] = (ssgSimpleState *)( s -> clone ( clone_flags )) ;
    else
      statelist [ i ] = s ;

	//~~ T.G. needs ref count incremented
	if (statelist [ i ] != NULL )      
	   statelist [ i ] -> ref();   

  }
}


ssgBase *ssgStateSelector::clone ( int clone_flags )
{
  ssgStateSelector *b = new ssgStateSelector ;
  b -> copy_from ( this, clone_flags ) ;
  return b ;
}


ssgStateSelector::ssgStateSelector () 
{ 
  type |= SSG_TYPE_STATESELECTOR ;
  nstates = 0 ;
  selection = -1 ; 
  statelist = NULL ;
}

ssgStateSelector::ssgStateSelector ( int ns ) 
{ 
  type |= SSG_TYPE_STATESELECTOR ;
  nstates   = ns ;
  selection = -1 ; 
  statelist = new ssgSimpleState * [ nstates ] () ; // MG-PLIB-019: defined pointer storage.

  for ( int i = 0 ; i < nstates ; i++ )
    statelist [ i ] = NULL ;
}

ssgStateSelector::~ssgStateSelector (void)
{
  //~~ T.G. deref states before deleting list
  for ( int i = 0 ; i < nstates ; i++ )
	  ssgDeRefDelete( statelist [ i ] );  
  delete [] statelist ;
}

void ssgStateSelector::selectStep ( unsigned int s )
{
  selection = s ;
}

unsigned int ssgStateSelector::getSelectStep (void)
{
  return selection ;
}

ssgSimpleState *ssgStateSelector::getCurrentStep  (void)
{
  return ( selection < 0 ||
           selection >= nstates ||
           statelist [ selection ] == NULL ) ? this : statelist[selection] ;
}

ssgSimpleState *ssgStateSelector::getStep ( int i )
{
  return ( i < 0 ||
           i >= nstates ||
           statelist [ i ] == NULL ) ? this : statelist[i] ;
}




void ssgStateSelector::setStep  (int i, ssgSimpleState *step)
{
  if ( i < 0 || i >= nstates ) return ;

  //~~ T.G. removed null test -- not necessary
  ssgDeRefDelete ( statelist[i] ) ;

  statelist [ i ] = step ;

  if ( step != NULL )
    step -> ref () ;
}


int ssgStateSelector::isTranslucent (void)
{
  return getCurrentStep()->isTranslucent() ;
}


void ssgStateSelector::setTranslucent (void)
{
  getCurrentStep()->setTranslucent() ;
}



void ssgStateSelector::setOpaque (void)
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setOpaque() ;
}


void ssgStateSelector::force (void)
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> force() ;
}

void ssgStateSelector::apply (void)
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> apply() ;
}


void ssgStateSelector:: care_about ( int mode )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> care_about (mode);
}


void ssgStateSelector::dont_care_about ( int mode )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> dont_care_about(mode);
}


int  ssgStateSelector::isEnabled ( GLenum mode )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  return s -> isEnabled(mode) ;
}

void ssgStateSelector::disable ( GLenum mode )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s  -> disable(mode) ;
}

void ssgStateSelector::enable ( GLenum mode )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> enable(mode) ;
}


void ssgStateSelector::setTexture ( char *fname, int _wrapu, int _wrapv,
				    int _mipmap )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s  -> setTexture ( fname, _wrapu, _wrapv, _mipmap ) ;
}

char *ssgStateSelector::getTextureFilename (void)  
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  return s -> getTextureFilename();
}

GLuint ssgStateSelector::getTextureHandle (void)  
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  return s -> getTextureHandle() ;
}

void ssgStateSelector::setTexture ( ssgTexture *tex )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setTexture(tex) ;
}

void ssgStateSelector::setTextureFilename ( char *fname )  
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  ssgTexture* texture ;
  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  texture = s -> getTexture () ;

  if ( texture != NULL )
    texture -> setFilename ( fname ) ;
}

void ssgStateSelector::setTexture ( GLuint tex )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  ssgTexture* texture ;
  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  texture = s -> getTexture () ;
  
  if ( texture != NULL )
    texture -> setHandle (tex) ;
}

void ssgStateSelector::setColourMaterial(GLenum which)
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setColourMaterial(which) ;
}

void ssgStateSelector::setMaterial ( GLenum which, float r, float g,
                                                   float b, float a )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setMaterial(which,r,g,b,a) ;
}


void ssgStateSelector::setMaterial ( GLenum which, sgVec4 rgba )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setMaterial(which,rgba) ;
}

float *ssgStateSelector::getMaterial ( GLenum which )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  return s -> getMaterial(which) ;
}

float ssgStateSelector::getShininess (void)
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  return s -> getShininess() ;
}

void ssgStateSelector::setShininess ( float sh )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setShininess(sh) ;
}

void ssgStateSelector::setShadeModel ( GLenum model )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setShadeModel(model) ;
}

void ssgStateSelector::setAlphaClamp ( float clamp )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> setAlphaClamp(clamp) ;
}

void ssgStateSelector::print ( FILE *fd, char *indent, int how_much )
{
  ssgSimpleState * s = getCurrentStep() ; // MG-PLIB-022: retain original virtual dispatch.

  // MG-PLIB-022: preserve virtual dispatch even when the selected state is self.
  s -> print(fd, indent, how_much) ;
}


int ssgStateSelector::load ( FILE *fd )
{
  int i ;
  int old_nstates=nstates; // MG-PLIB-019: safe native replacement of prior list.

  _ssgReadInt ( fd, & nstates   ) ;
  _ssgReadInt ( fd, & selection ) ;

  if (statelist != NULL)
  {
    for ( i = 0 ; i < old_nstates ; i++ )
      ssgDeRefDelete( statelist [ i ] );
    delete [] statelist ;
  }

  statelist = new ssgSimpleState * [ nstates ] () ; // MG-PLIB-019: defined pointer storage.
  for ( i = 0 ; i < nstates ; i++ )
  {
    ssgBase* loaded=NULL;int needs_load=0;
    if(!_ssgV0ReadObjectHeader(fd,&loaded,ssgTypeState(),4,&needs_load))return FALSE;
    statelist[i]=(ssgSimpleState*)loaded;
    if(statelist[i])statelist[i]->ref(); // Original publishes/refs before virtual load.
    if(needs_load>0 && !loaded->load(fd))return FALSE;
  }

  return ssgSimpleState::load(fd) ;
}


int ssgStateSelector::save ( FILE *fd )
{

  _ssgWriteInt ( fd, nstates   ) ;
  _ssgWriteInt ( fd, selection ) ;
  for ( int i = 0 ; i < nstates ; i++ )
  {
    if ( ! _ssgSaveObject ( fd, statelist[i] ) )
      return FALSE ;
  }

  return ssgSimpleState::save(fd) ;
}

