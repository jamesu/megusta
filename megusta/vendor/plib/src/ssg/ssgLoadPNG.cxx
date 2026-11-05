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

     $Id: ssgLoadPNG.cxx,v 1.7 2001/04/12 03:38:15 deca Exp $
*/


#include "ssgLocal.h"
#include "multi_texture.h"
extern bool _ssgTextureHasAlpha; // MG-PLIB-037: original shared load result.

#ifdef SSG_LOAD_PNG_SUPPORTED

#include <GL/glpng.h> // MEGUSTA MOD MG-PLIB-004: distribution header case

bool ssgLoadPNG ( const char *fname, ssgTextureInfo* info )
{
  int width=0,height=0,alpha=0;
  if (!megusta::pix::loadBoundTexture(fname,&width,&height,&alpha)) {
    ulSetError ( UL_WARNING, "ssgLoadTexture: Failed to load '%s'.", fname ) ;
    return false ;
  }
  _ssgTextureHasAlpha=alpha!=0;
  if ( info != NULL )
  {
    info -> width = width ;
    info -> height = height ;
    info -> depth = 8 ;
    info -> alpha = alpha ;
  }
  return true ;
}

#else

bool ssgLoadPNG ( const char *fname, ssgTextureInfo* info )
{
  ulSetError ( UL_WARNING, "ssgLoadTexture: '%s' - you need glpng for PNG format support",
        fname ) ;
  return false ;
}

#endif
