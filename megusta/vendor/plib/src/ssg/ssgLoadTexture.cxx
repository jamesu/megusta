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

     $Id: ssgLoadTexture.cxx,v 1.7 2001/04/12 03:38:15 deca Exp $
*/


#include "ssgLocal.h"
#include <string>
#include "pix_texture.h"


struct _ssgTextureFormat
{
  const char *extension ;
  bool (*loadfunc) ( const char *, ssgTextureInfo* info ) ;
} ;


// MG-PLIB-040: fixed executable registry, available without ssgInit.
extern bool _ssgTextureHasAlpha;
static bool loadPixImage(const char* filename,ssgTextureInfo* info)
{
  megusta::pix::TextureInfo result{};
  if(!megusta::pix::load_file(filename,&result,true)){
    ulSetError(UL_WARNING,"ssgLoadTexture: Failed to load '%s'.",filename);return false;
  }
  _ssgTextureHasAlpha=result.alpha>0;
  if(info){info->width=result.width;info->height=result.height;info->depth=result.bits_per_pixel;info->alpha=result.alpha;}
  return true;
}
static const _ssgTextureFormat formats[]={
  {".pix",loadPixImage},{".tga",ssgLoadTGA},{".png",ssgLoadPNG}
};
using EmbeddedPixLoader=bool(*)(const char*,ssgTextureInfo*);
EmbeddedPixLoader _ssgEmbeddedPixLoader=nullptr;

static int total_texels_loaded = 0 ;


int ssgGetNumTexelsLoaded ()
{
  return total_texels_loaded ;
}


// MG-PLIB-037: original upload delegates to the glpng algorithm. Image storage
// belongs to the caller; there are no proxy uploads or alignment changes here.
extern "C" void megusta_glpng_buildMipmaps(GLint,GLint,GLint,GLenum,const unsigned char*,int);
static void ssgLoadDummyTexture(ssgTextureInfo* info);
bool _ssgTextureHasAlpha=false;
std::string _ssgImageFilename;
bool ssgMakeMipMaps(GLubyte* image,int width,int height,int channels)
{
  if((width & (width-1)) || (height & (height-1)))
  {
    ulSetError(UL_WARNING,"%s: Map is not a power-of-two in size!",_ssgImageFilename.c_str());
    ssgLoadDummyTexture(NULL);
    return false;
  }
  GLenum format=channels==1?GL_LUMINANCE:channels==2?GL_LUMINANCE_ALPHA:channels==3?GL_RGB:GL_RGBA;
  megusta_glpng_buildMipmaps(channels,width,height,format,image,1);
  return true;
}

static void ssgLoadDummyTexture ( ssgTextureInfo* info )
{
  GLubyte *image = new GLubyte [ 2 * 2 * 3 ] ;

  /* Red and white chequerboard */

  image [ 0 ] = 255 ; image [ 1 ] =  0  ; image [ 2 ] =  0  ;
  image [ 3 ] = 255 ; image [ 4 ] = 255 ; image [ 5 ] = 255 ;
  image [ 6 ] = 255 ; image [ 7 ] = 255 ; image [ 8 ] = 255 ;
  image [ 9 ] = 255 ; image [ 10] =  0  ; image [ 11] =  0  ;

  if ( info != NULL )
  {
    info -> width = 2 ;
    info -> height = 2 ;
    info -> depth = 3 ;
    info -> alpha = 0 ;
  }

  ssgMakeMipMaps ( image, 2, 2, 3 ) ;
  delete [] image ; // MG-PLIB-037: avoid the original fallback allocation leak.
}


bool ssgLoadTexture ( const char * filename, ssgTextureInfo* info )
{
  _ssgTextureHasAlpha=false; // MG-PLIB-037: original shared per-load flag.
  if ( info != NULL )
  {
    info -> width = 0 ;
    info -> height = 0 ;
    info -> depth = 0 ;
    info -> alpha = 0 ;
  }

  if ( filename == NULL || *filename == '\0' )
    return false ;

  // The ASX reader installs this only while embedded texture records are live.
  if(_ssgEmbeddedPixLoader && _ssgEmbeddedPixLoader(filename,info))return true;
  char path[1024];ssgGetCurrentOptions()->makeTexturePath(path,filename);filename=path;

  //find extension
  const char *extn = & ( filename [ strlen(filename) ] ) ;
  while ( extn != filename && *extn != '/' && *extn != '.' )
    extn-- ;

  if ( *extn != '.' )
  {
    ulSetError ( UL_WARNING, "ssgLoadTexture: Cannot determine file type for '%s'", filename );
    return false ;
  }

  for(const auto& format:formats)
    if(ulStrNEqual(extn,format.extension,strlen(format.extension)))
      return format.loadfunc(filename,info); // No extra fallback after a recognized loader fails.

  ulSetError ( UL_WARNING, "ssgLoadTexture: Unrecognised file type '%s'", extn ) ;
  ssgLoadDummyTexture ( info ) ;
  return false ;
}
