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

     $Id: ssgBase.cxx,v 1.15 2001/04/29 16:07:15 sjbaker Exp $
*/


#include "ssgLocal.h"

static int next_unique_id = 1 ;

void *ssgBase::operator new ( size_t sz )
{
  return malloc ( sz ) ;
}

void ssgBase::operator delete ( void *ptr )
{
  free ( ptr ) ;
}

void ssgBase::copy_from ( ssgBase *src, int clone_flags )
{
  // MEGUSTA MOD MG-PLIB-001: binary 00466620 copy semantics.
  if ( type != src -> getType () )
  {
    ulSetError ( UL_FATAL, "SSG: copy_from type mismatch" ) ;
    abort () ; // Preserve unconditional failure even with a returning error callback.
  }
  type = src -> getType () ;

  // type  = src -> getType () ; - cannot change the virtual function table...
  spare = src -> getSpare () ;
  refc = 0 ; // MEGUSTA MOD MG-PLIB-001: after getSpare, as in 00466620.
  // refc  = 0 ; - okay?

  if ( clone_flags & SSG_CLONE_USERDATA )
    setUserData ( src -> getUserData () ) ;
  else
    setUserData ( NULL ) ;

  setName ( src -> getName () ) ;
}



ssgBase *ssgBase::clone ( int /* clone_flags */ )
{
  ulSetError ( UL_FATAL, "SSG: Can't clone abstract SSG class objects" ) ;
  return NULL ;
}

ssgBase:: ssgBase (void)
{
  spare = refc = 0 ;
  type      = SSG_TYPE_BASE ;
  unique    = next_unique_id++ ;
  user_data = NULL ;
  name      = NULL ;
}

ssgBase::~ssgBase (void)
{
  ssgDeRefDelete ( user_data ) ;                                              

  deadBeefCheck () ;
  assert ( refc == 0 ) ;

  delete [] name ; // MEGUSTA MOD MG-PLIB-004: name is allocated with new[]

  /*
    Set the type of deleted nodes to 0xDeadBeef so we'll
    stand a chance of detecting re-use of deleted nodes.
  */

  type = (int) 0xDeadBeef ;
}

 
void ssgBase::setName ( const char *nm )
{
  delete [] name ; // MEGUSTA MOD MG-PLIB-004: name is allocated with new[]
 
  if ( nm == NULL )
    name = NULL ;
  else
  {
    name = new char [ strlen ( nm ) + 1 ] ;
    strcpy ( name, nm ) ;
  }
}
                                                                                
void ssgBase::zeroSpareRecursive (){ zeroSpare () ; }
void ssgBase::zeroSpare ()         { spare = 0    ; }
void ssgBase::incSpare  ()         { spare++      ; }
void ssgBase::setSpare  ( int ss ) { spare = ss   ; }
int  ssgBase::getSpare  ()         { return spare ; }


void ssgBase::print ( FILE *fd, char *indent, int /* how_much */ )
{
  // MG-PLIB-018: 00466840 prints all diagnostic fields unconditionally.
  fprintf ( fd, "%s%s: Ref Count=%d\n", indent, getTypeName(), getRef () ) ;
  fprintf ( fd, "%s  Name = \"%s\"\n", indent, getPrintableName() ) ;
  fprintf ( fd, "%s  Userdata = %p\n", indent, getUserData() ) ;
  deadBeefCheck () ;
}

int ssgBase::load ( FILE *fd )
{ 
  delete [] name ; // MEGUSTA MOD MG-PLIB-004: name is allocated with new[]
  name = NULL ;
  setSpare(_ssgV0NextKey());_ssgV0Register(getSpare(),this); // MG-PLIB-019
  _ssgReadString ( fd, &name ) ;
  return ! _ssgReadError () ;
}

int ssgBase::save ( FILE *fd )
{
  setSpare(_ssgV0NextKey()); // MG-PLIB-019: assigned when base payload is reached.
  _ssgWriteString ( fd, name ) ;
  return ! _ssgWriteError () ;
}


