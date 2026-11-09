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

     $Id: fntTXF.cxx,v 1.9 2001/04/09 00:24:58 deca Exp $
*/


#include "fntLocal.h"

#if !defined(WIN32) && !defined(__EMSCRIPTEN__)
#  ifndef macintosh
#    include <GL/glx.h>
#  else
#    include <agl.h>
#  endif
#endif

// MG-PLIB-056: executable delegates filtered mipmaps to glpng and has no
// GL-context validation gate. The returned texture image remains caller-owned.
void *curr_image_fd ;
namespace {
void* stdioOpen(const char* path,const char* mode) { return fopen(path,mode); }
size_t stdioRead(void* out,size_t size,size_t count,void* file) { return fread(out,size,count,static_cast<FILE*>(file)); }
int stdioClose(void* file) { return fclose(static_cast<FILE*>(file)); }
void* (*fontOpen)(const char*,const char*)=stdioOpen;
size_t (*fontRead)(void*,size_t,size_t,void*)=stdioRead;
int (*fontClose)(void*)=stdioClose;
}
void fntSetFileCallbacks(void* (*open)(const char*,const char*),size_t (*read)(void*,size_t,size_t,void*),int (*close)(void*)) {
  fontOpen=open?open:stdioOpen; fontRead=read?read:stdioRead; fontClose=close?close:stdioClose;
}
size_t fntFileRead(void* output,size_t size,size_t count,void* file) { return fontRead(output,size,count,file); }
int isSwapped = FALSE ;
extern "C" void megusta_glpng_buildMipmaps(GLint,GLint,GLint,GLenum,const unsigned char*,int);
extern "C" unsigned char megusta_glpng_gammaByte(unsigned char);

int fntTexFont::loadTXF ( const char *fname, GLenum mag, GLenum min )
{
  void *fd ;

  if ( (fd = fontOpen ( fname, "rb" )) == NULL )
  {
    ulSetError ( UL_WARNING,
      "fntLoadTXF: Failed to open '%s' for reading.", fname ) ;
    return FNT_FALSE ;
  }

  curr_image_fd = fd ;

  unsigned char magic [ 4 ] ;

  if ( (int)fntFileRead ( &magic, sizeof (unsigned int), 1, fd ) != 1 )
  {
    ulSetError ( UL_WARNING,
      "fntLoadTXF: '%s' an empty file!", fname ) ;
    fontClose(fd); return FNT_FALSE ;
  }

  if ( magic [ 0 ] != 0xFF || magic [ 1 ] != 't' ||
       magic [ 2 ] != 'x'  || magic [ 3 ] != 'f' )
  {
    ulSetError ( UL_WARNING,
      "fntLoadTXF: '%s' is not a 'txf' font file.", fname ) ;
    fontClose(fd); return FNT_FALSE ;
  }

  isSwapped  = FALSE ;
  int endianness  = _fnt_readInt () ;

  isSwapped  = ( endianness != 0x12345678 ) ;

  int format      = _fnt_readInt () ;
  int tex_width   = _fnt_readInt () ;
  int tex_height  = _fnt_readInt () ;
  int max_height  = _fnt_readInt () ;
                    _fnt_readInt () ;
  int num_glyphs  = _fnt_readInt () ;

  int w = tex_width  ;
  int h = tex_height ;

  float xstep = 0.5f / (float) w ;
  float ystep = 0.5f / (float) h ;
  if (min == GL_NEAREST || min == GL_LINEAR) xstep = ystep = 0.0f ;

  int i, j ;

  /*
    Load the TXF_Glyph array
  */

  TXF_Glyph glyph ;

  for ( i = 0 ; i < num_glyphs ; i++ )
  {
    glyph . ch      = _fnt_readShort () ;

    glyph .  w      = _fnt_readByte  () ;
    glyph .  h      = _fnt_readByte  () ;
    glyph . x_off   = _fnt_readByte  () ;
    glyph . y_off   = _fnt_readByte  () ;
    glyph . step    = _fnt_readByte  () ;
    glyph . unknown = _fnt_readByte  () ;
    glyph . x       = _fnt_readShort () ;
    glyph . y       = _fnt_readShort () ;

    setGlyph ( (char) glyph.ch,
          (float)  glyph.x                 / (float) w + xstep,
          (float)( glyph.x + glyph.w )     / (float) w + xstep,
          (float)  glyph.y                 / (float) h + ystep,
          (float)( glyph.y + glyph.h )     / (float) h + ystep,
          (float)  glyph.x_off             / (float) max_height,
          (float)( glyph.x_off + glyph.w ) / (float) max_height,
          (float)  glyph.y_off             / (float) max_height,
          (float)( glyph.y_off + glyph.h ) / (float) max_height,
          (float) glyph.step / (float) max_height ) ; // MG-PLIB-014
  }

  // MG-PLIB-056: the game does not forcibly discard a space glyph.

  /*
    Load the image part of the file
  */

  int ntexels = w * h ;

  unsigned char *teximage  ;
  unsigned char *texbitmap ;

  switch ( format )
  {
    case FNT_BYTE_FORMAT:
      {
        unsigned char *orig = new unsigned char [ ntexels ] ;

        if ( (int)fntFileRead ( orig, 1, ntexels, fd ) != ntexels )
        {
          ulSetError ( UL_WARNING,
            "fntLoadTXF: Premature EOF in '%s'.", fname ) ;
          delete [] orig; fontClose(fd); return FNT_FALSE ;
        }

        teximage = new unsigned char [ 2 * ntexels ] ;

        for ( i = 0 ; i < ntexels ; i++ )
        {
          teximage [ i*2     ] = megusta_glpng_gammaByte(orig [ i ]) ; // MG-GLPNG-002
          teximage [ i*2 + 1 ] = orig [ i ] ;
        }

        delete [] orig ;
      }
      break ;
   
    case FNT_BITMAP_FORMAT:
      {
        int stride = (w + 7) >> 3;

        texbitmap = new unsigned char [ stride * h ] ;

        if ( (int)fntFileRead ( texbitmap, 1, stride * h, fd ) != stride * h )
        {
          delete [] texbitmap ;
          ulSetError ( UL_WARNING,
            "fntLoadTXF: Premature EOF in '%s'.", fname ) ;
          fontClose(fd); return FNT_FALSE ;
      	}

        teximage = new unsigned char [ 2 * ntexels ] ;

        for ( i = 0 ; i < 2 * ntexels ; i++ )
	  teximage [ i ] = 0 ;

        for (i = 0; i < h; i++)
	  for (j = 0; j < w; j++)
	    if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7)))
	    {
	      teximage[(i * w + j) * 2    ] = 255;
	      teximage[(i * w + j) * 2 + 1] = 255;
	    }

        delete [] texbitmap ;
      }
      break ;

    default:
      ulSetError ( UL_WARNING,
        "fntLoadTXF: Unrecognised format type in '%s'.", fname ) ;
      fontClose(fd); return FNT_FALSE ;
  }

  fontClose ( fd ) ;

  fixed_pitch = FALSE ;

#ifdef GL_VERSION_1_1
  glGenTextures ( 1, & texture ) ;
  glBindTexture ( GL_TEXTURE_2D, texture ) ;
#else
  /* This is only useful on some ancient SGI hardware */
  glGenTexturesEXT ( 1, & texture ) ;
  glBindTextureEXT ( GL_TEXTURE_2D, texture ) ;
#endif

  if (min == GL_NEAREST || min == GL_LINEAR) {
    GLint alignment; glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,2,w,h,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,teximage);
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
  } else megusta_glpng_buildMipmaps(2,w,h,GL_LUMINANCE_ALPHA,teximage,1);
  delete [] teximage ;

  glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;

  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag ) ;
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min ) ;
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP ) ;
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP ) ;
#ifdef GL_VERSION_1_1
  glBindTexture ( GL_TEXTURE_2D, 0 ) ;
#else
  glBindTextureEXT ( GL_TEXTURE_2D, 0 ) ;
#endif
  
  return FNT_TRUE ;
}


