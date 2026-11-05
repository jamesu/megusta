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

     $Id: ssgLoadTGA.cxx,v 1.6 2001/04/12 03:38:15 deca Exp $
*/


#include "ssgLocal.h"

// MG-PLIB-037: adapted from executable 004b5fe0, not upstream 1.3.2's
// true-colour decoder. Only uncompressed 8-bit, 256-entry paletted TGA is read.
#include <vector>
#include <algorithm>
#include <string>
extern bool _ssgTextureHasAlpha;
extern std::string _ssgImageFilename;
static unsigned tgaWord(const unsigned char* p){return p[0]|(unsigned(p[1])<<8);}
bool ssgLoadTGA(const char* filename,ssgTextureInfo* info)
{
  _ssgImageFilename=filename;
  FILE* file=fopen(filename,"rb");
  auto fallback=[&](){GLubyte pixels[]={255,0,0,255,255,255,255,255,255,255,0,0};ssgMakeMipMaps(pixels,2,2,3);};
  if(!file){ulSetError(UL_WARNING,"ssgLoadTexture: Failed to load '%s'.",filename);fallback();return false;}
  unsigned char h[18]={};fread(h,1,18,file);
  unsigned width=tgaWord(h+12),height=tgaWord(h+14),stride=h[7]/8;
  if(h[16]!=8 || h[1]!=1 || tgaWord(h+3)!=0 || tgaWord(h+5)!=256 || h[2]!=1 ||
     (h[17]&16) || tgaWord(h+8)!=0 || tgaWord(h+10)!=0 || !width || !height ||
     (h[7]!=24 && h[7]!=32))
  {
    fclose(file);ulSetError(UL_WARNING,"ssgLoadTexture: Unsupported Targa format '%s'.",filename);
    fallback();return false;
  }
  fseek(file,h[0],SEEK_CUR);
  std::vector<unsigned char> palette(256*stride);
  fread(palette.data(),256,stride,file);
  bool transparent=false,partial=false;
  for(unsigned i=0;i<256;++i){std::swap(palette[i*stride],palette[i*stride+2]);
    if(stride==4){if(palette[i*stride+3]==0)transparent=true;else if(palette[i*stride+3]!=255)partial=true;}}
  unsigned channels=transparent||partial?4:3;
  std::vector<unsigned char> image(size_t(width)*height*channels),row(width);
  for(unsigned y=0;y<height;++y)
  {
    fread(row.data(),1,width,file);
    unsigned destination=(h[17]&32)?y:height-1-y;
    for(unsigned x=0;x<width;++x)
      std::copy_n(palette.data()+row[x]*stride,channels,image.data()+(size_t(destination)*width+x)*channels);
  }
  ssgMakeMipMaps(image.data(),width,height,channels);
  // Original successful path leaks FILE; closing is an explicit portability fix.
  fclose(file);
  _ssgTextureHasAlpha=transparent||partial;
  if(info){info->width=width;info->height=height;info->depth=channels;info->alpha=_ssgTextureHasAlpha;}
  return true;
}
