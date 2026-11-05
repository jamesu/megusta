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

     $Id: ssgTexTrans.cxx,v 1.7 2001/04/12 03:38:16 deca Exp $
*/


#include "ssgLocal.h"

// MG-PLIB-025: adapted original branch-only class, 0048e600..0048e70e.
void ssgTexTrans::copy_from ( ssgTexTrans *src, int clone_flags )
{
  ssgBranch::copy_from(src,clone_flags);
  legacy_copy_word=src->legacy_copy_word;
}

ssgBase *ssgTexTrans::clone ( int clone_flags )
{
  ssgTexTrans *copy=new ssgTexTrans;
  copy->copy_from(this,clone_flags);
  return copy;
}

ssgTexTrans::ssgTexTrans ()
{
  type |= SSG_TYPE_TEXTRANS;
  // Original clone allocates 100 bytes but copy reads the word at +100.
  // Give that otherwise out-of-bounds word defined native storage.
  legacy_copy_word=0;
}

ssgTexTrans::~ssgTexTrans () {}

int ssgTexTrans::load ( FILE *fd )
{
  // MG-PLIB-019: adapted original version-0 record layout.
  return ssgBranch::load(fd); // Original TexTrans stores no transform matrix.
}

int ssgTexTrans::save ( FILE *fd )
{
  // MG-PLIB-019: adapted original version-0 record layout.
  return ssgBranch::save(fd); // Original TexTrans stores no transform matrix.
}



