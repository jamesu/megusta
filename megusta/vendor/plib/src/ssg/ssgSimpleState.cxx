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

     $Id: ssgSimpleState.cxx,v 1.20 2001/04/12 03:38:15 deca Exp $
*/


#include "ssgLocal.h"

void ssgSimpleState::copy_from ( ssgSimpleState *src, int clone_flags )
{
  ssgState::copy_from ( src, clone_flags ) ;

  ssgTexture *tex = src -> getTexture () ;

  ssgDeRefDelete(texture); 
  
  if ( tex != NULL && ( clone_flags & SSG_CLONE_TEXTURE ) )
    texture = (ssgTexture *)( tex -> clone ( clone_flags ) ) ;
  else
    texture = tex ;
  
  if (texture != NULL)  
    texture->ref(); 

  dont_care      = src -> dont_care ;
  enables        = src -> enables   ;

  colour_material_mode = src -> colour_material_mode ;

  sgCopyVec4 ( specular_colour, src -> specular_colour ) ;
  sgCopyVec4 ( emission_colour, src -> emission_colour ) ;
  sgCopyVec4 (  ambient_colour, src ->  ambient_colour ) ;
  sgCopyVec4 (  diffuse_colour, src ->  diffuse_colour ) ;

  shade_model = src -> shade_model ;
  shininess   = src -> shininess   ;
  alpha_clamp = src -> alpha_clamp ;
}

ssgBase *ssgSimpleState::clone ( int clone_flags )
{
  ssgSimpleState *b = new ssgSimpleState ;
  b -> copy_from ( this, clone_flags ) ;
  return b ;
}


void _ssgForceLineState ()
{
  _ssgCurrentContext->getState()->enables &= ~((1<<SSG_GL_TEXTURE_EN) |
                                (1<<SSG_GL_LIGHTING_EN)|
                                (1<<SSG_GL_COLOR_MATERIAL_EN)) ;
  glDisable ( GL_TEXTURE_2D ) ;
  glDisable ( GL_COLOR_MATERIAL ) ;
  glDisable ( GL_LIGHTING  ) ;
  glDisable ( GL_DEPTH_TEST ) ;
}


ssgSimpleState::ssgSimpleState ( int /* I_am_currstate */ )
{
  type |= SSG_TYPE_SIMPLESTATE ;
  enables = 0 ; // MEGUSTA MOD MG-PLIB-006: defined backing bits before read/modify/write.
  texture  = NULL ;
  dont_care = 0xFFFFFFFF ;
  disable ( GL_TEXTURE_2D  ) ;
  enable  ( GL_CULL_FACE   ) ;
  enable  ( GL_COLOR_MATERIAL ) ;
  enable  ( GL_LIGHTING ) ;
  disable ( GL_BLEND ) ;
  disable ( GL_ALPHA_TEST ) ; // MEGUSTA MOD MG-PLIB-006: binary 0046c1c0.
  setShadeModel ( GL_SMOOTH ) ;
}

ssgSimpleState::ssgSimpleState (void)
{
  type |= SSG_TYPE_SIMPLESTATE ;
  enables = 0 ; // MEGUSTA MOD MG-PLIB-006: ignored bits have defined storage.

  texture  = NULL ;
  dont_care = 0xFFFFFFFF ;
}

ssgSimpleState::~ssgSimpleState (void)
{
  ssgDeRefDelete ( texture ) ;
}

void ssgSimpleState::apply (void)
{
  // MG-PLIB-036: original material updates precede capability changes.
  bool colour_mode_changed=false;
  if((~dont_care & (1<<SSG_GL_COLOR_MATERIAL)) &&
     _ssgCurrentContext->getState()->colour_material_mode!=colour_material_mode)
  {
    glColorMaterial(GL_FRONT_AND_BACK,(GLenum)colour_material_mode);
    _ssgCurrentContext->getState()->colour_material_mode=colour_material_mode;
    colour_mode_changed=true;
  }

  if ( ~ dont_care & ( (1<<SSG_GL_DIFFUSE       ) |
                       (1<<SSG_GL_AMBIENT       ) |
                       (1<<SSG_GL_SPECULAR      ) |
                       (1<<SSG_GL_EMISSION      ) |
                       (1<<SSG_GL_SHININESS     ) ) )
  {
    if ( ( ~ dont_care & (1<<SSG_GL_SHININESS) ) &&
      _ssgCurrentContext->getState()->shininess != shininess )
    {
      glMaterialf ( GL_FRONT_AND_BACK, GL_SHININESS, shininess ) ;
      _ssgCurrentContext->getState()->shininess = shininess ;
    }

    if ( ( ~ dont_care & (1<<SSG_GL_SPECULAR) ) &&
         (colour_mode_changed || ! sgEqualVec3 ( _ssgCurrentContext->getState()->specular_colour, specular_colour )) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_SPECULAR, specular_colour ) ;
      sgCopyVec3 ( _ssgCurrentContext->getState()->specular_colour, specular_colour ) ;
    }

    if ( ( ~ dont_care & (1<<SSG_GL_EMISSION) ) &&
         (colour_mode_changed || ! sgEqualVec3 ( _ssgCurrentContext->getState()->emission_colour, emission_colour )) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_EMISSION, emission_colour ) ;
      sgCopyVec3 ( _ssgCurrentContext->getState()->emission_colour, emission_colour ) ;
    }

    if ( ( ~ dont_care & (1<<SSG_GL_AMBIENT) ) &&
         (colour_mode_changed || ! sgEqualVec3 ( _ssgCurrentContext->getState()->ambient_colour, ambient_colour )) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT, ambient_colour ) ;
      sgCopyVec3 ( _ssgCurrentContext->getState()->ambient_colour, ambient_colour ) ;
    }

    if ( ( ~ dont_care & (1<<SSG_GL_DIFFUSE) ) &&
         (colour_mode_changed || ! sgEqualVec4 ( _ssgCurrentContext->getState()->diffuse_colour, diffuse_colour )) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_colour ) ;
      sgCopyVec4 ( _ssgCurrentContext->getState()->diffuse_colour, diffuse_colour ) ;
    }
  }

  int turn_on=~dont_care & enables & ~_ssgCurrentContext->getState()->enables & 0x3f;
  int turn_off=~dont_care & ~enables & _ssgCurrentContext->getState()->enables & 0x3f;
  (*__ssgEnableTable[turn_on])();
  (*__ssgDisableTable[turn_off])();
  _ssgCurrentContext->getState()->enables |= turn_on;
  _ssgCurrentContext->getState()->enables &= ~turn_off;

  if ( ( ~ dont_care & (1<<SSG_GL_TEXTURE) ) && 
    _ssgCurrentContext->getState()->getTexture() != getTexture() )
  {
    stats_bind_textures++ ;
#ifdef GL_VERSION_1_1
    glBindTexture ( GL_TEXTURE_2D, getTextureHandle() ) ;
#else
    /* For ancient SGI machines */
    glBindTextureEXT ( GL_TEXTURE_2D, getTextureHandle() ) ;
#endif
    _ssgCurrentContext->getState()->setTexture ( getTexture () ) ;
  }

  if ( ( ~ dont_care & (1<<SSG_GL_SHADE_MODEL) ) &&
    _ssgCurrentContext->getState()->shade_model != shade_model )
  {
    glShadeModel ( shade_model ) ;
    _ssgCurrentContext->getState()->shade_model = shade_model ;
  }

  if ( ( ~ dont_care & (1<<SSG_GL_ALPHA_TEST) ) &&
    _ssgCurrentContext->getState()->alpha_clamp != alpha_clamp )
  {
    glAlphaFunc ( GL_GREATER, alpha_clamp ) ;
    _ssgCurrentContext->getState()->alpha_clamp = alpha_clamp ;
  }

}

void ssgSimpleState::force (void)
{
  // MG-PLIB-036: no upstream colour-cache invalidation.
  if ( ~ dont_care & ( (1<<SSG_GL_COLOR_MATERIAL ) |
                       (1<<SSG_GL_DIFFUSE        ) |
                       (1<<SSG_GL_AMBIENT        ) |
                       (1<<SSG_GL_SPECULAR       ) |
                       (1<<SSG_GL_EMISSION       ) |
                       (1<<SSG_GL_SHININESS      ) ) )
  {
    if ( ~ dont_care & (1<<SSG_GL_COLOR_MATERIAL ) )
    {
      glColorMaterial ( GL_FRONT_AND_BACK, (GLenum) colour_material_mode ) ;
      _ssgCurrentContext->getState()->colour_material_mode = colour_material_mode ;
    }

    if ( ~ dont_care & (1<<SSG_GL_SHININESS ) )
    {
      glMaterialf ( GL_FRONT_AND_BACK, GL_SHININESS, shininess ) ;
      _ssgCurrentContext->getState()->shininess = shininess ;
    }

    if ( ~ dont_care & (1<<SSG_GL_DIFFUSE ) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_colour ) ;
      sgCopyVec4 ( _ssgCurrentContext->getState()->diffuse_colour, diffuse_colour ) ;
    }

    if ( ~ dont_care & (1<<SSG_GL_EMISSION ) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_EMISSION, emission_colour ) ;
      sgCopyVec3 ( _ssgCurrentContext->getState()->emission_colour, emission_colour ) ;
    }

    if ( ~ dont_care & (1<<SSG_GL_AMBIENT ) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT, ambient_colour ) ;
      sgCopyVec3 ( _ssgCurrentContext->getState()->ambient_colour, ambient_colour ) ;
    }

    if ( ~ dont_care & (1<<SSG_GL_SPECULAR ) )
    {
      glMaterialfv ( GL_FRONT_AND_BACK, GL_SPECULAR, specular_colour ) ;
      sgCopyVec3 ( _ssgCurrentContext->getState()->specular_colour, specular_colour ) ;
    }
  }

  int turn_on=~dont_care & enables & 0x3f;
  int turn_off=~dont_care & ~enables & 0x3f;
  (*__ssgEnableTable[turn_on])();
  (*__ssgDisableTable[turn_off])();
  _ssgCurrentContext->getState()->enables |= turn_on;
  _ssgCurrentContext->getState()->enables &= ~turn_off;

  if ( ~ dont_care & (1<<SSG_GL_TEXTURE ) )
  {
    stats_bind_textures++ ;
#ifdef GL_VERSION_1_1
    glBindTexture ( GL_TEXTURE_2D, getTextureHandle() ) ;
#else
    /* For ancient SGI machines */
    glBindTextureEXT ( GL_TEXTURE_2D, getTextureHandle() ) ;
#endif
    _ssgCurrentContext->getState()->setTexture ( getTexture () ) ;
  }

  if ( ~ dont_care & (1<<SSG_GL_SHADE_MODEL ) )
  {
    glShadeModel ( shade_model ) ;
    _ssgCurrentContext->getState()->shade_model = shade_model ;
  }

  if ( ~ dont_care & (1<<SSG_GL_ALPHA_TEST ) )
  {
    glAlphaFunc ( GL_GREATER, alpha_clamp ) ;
    _ssgCurrentContext->getState()->alpha_clamp = alpha_clamp ;
  }

}


int ssgSimpleState::isEnabled ( GLenum mode )
{
  switch ( mode )
  {
    case GL_TEXTURE_2D        :
      return enables & (1<<SSG_GL_TEXTURE_EN) ;

    case GL_CULL_FACE      :
      return enables & (1<<SSG_GL_CULL_FACE_EN) ;

    case GL_COLOR_MATERIAL :
      return enables & (1<<SSG_GL_COLOR_MATERIAL_EN) ;

    case GL_LIGHTING :
      return enables & (1<<SSG_GL_LIGHTING_EN) ;

    case GL_BLEND          :
      return enables & (1<<SSG_GL_BLEND_EN) ;

    case GL_ALPHA_TEST     :
      return enables & (1<<SSG_GL_ALPHA_TEST_EN) ;

    default :
      return FALSE ;
  }
}



void ssgSimpleState::disable ( GLenum mode )
{
  switch ( mode )
  {
    case GL_TEXTURE_2D        :
      enables &= ~ (1<<SSG_GL_TEXTURE_EN) ;
      care_about ( SSG_GL_TEXTURE_EN ) ;
      break ;

    case GL_CULL_FACE      :
      enables &= ~ (1<<SSG_GL_CULL_FACE_EN) ;
      care_about ( SSG_GL_CULL_FACE_EN ) ;
      break ;

    case GL_COLOR_MATERIAL :
      enables &= ~ (1<<SSG_GL_COLOR_MATERIAL_EN) ;
      care_about ( SSG_GL_COLOR_MATERIAL_EN ) ;
      break ;

    case GL_LIGHTING :
      enables &= ~ (1<<SSG_GL_LIGHTING_EN) ;
      care_about ( SSG_GL_LIGHTING_EN ) ;
      break ;

    case GL_BLEND          :
      enables &= ~ (1<<SSG_GL_BLEND_EN) ;
      care_about ( SSG_GL_BLEND_EN ) ;
      break ;

    case GL_ALPHA_TEST     :
      enables &= ~ (1<<SSG_GL_ALPHA_TEST_EN) ;
      care_about ( SSG_GL_ALPHA_TEST_EN ) ;
      break ;

    default :
      ulSetError ( UL_WARNING, "Illegal mode passed to ssgSimpleState::disable(%d)",
			       mode ) ;
      break ; 
  }
}

void ssgSimpleState::enable  ( GLenum mode )
{
  switch ( mode )
  {
    case GL_TEXTURE_2D        :
      enables |=  (1<<SSG_GL_TEXTURE_EN) ;
      care_about ( SSG_GL_TEXTURE_EN ) ;
      break ;

    case GL_CULL_FACE      :
      enables |=  (1<<SSG_GL_CULL_FACE_EN) ;
      care_about ( SSG_GL_CULL_FACE_EN ) ;
      break ;

    case GL_COLOR_MATERIAL :
      enables |=  (1<<SSG_GL_COLOR_MATERIAL_EN) ;
      care_about ( SSG_GL_COLOR_MATERIAL_EN ) ;
      break ;

    case GL_BLEND          :
      enables |=  (1<<SSG_GL_BLEND_EN) ;
      care_about ( SSG_GL_BLEND_EN ) ;
      break ;

    case GL_ALPHA_TEST     :
      enables |=  (1<<SSG_GL_ALPHA_TEST_EN) ;
      care_about ( SSG_GL_ALPHA_TEST_EN ) ;
      break ;

    case GL_LIGHTING :
      enables |= (1<<SSG_GL_LIGHTING_EN) ;
      care_about ( SSG_GL_LIGHTING_EN ) ;
      break ;

    default :
      ulSetError ( UL_WARNING,
	     "Illegal mode passed to ssgSimpleState::enable(%d)",
			       mode ) ;
      break ; 
  }
}


int ssgSimpleState::load ( FILE *fd )
{
  // MG-PLIB-019: adapted original version-0 record layout.
  int marker=0,token=0;_ssgReadInt(fd,&marker);
  _ssgReadInt(fd,&dont_care);_ssgReadInt(fd,&enables);
  _ssgReadInt(fd,&token);colour_material_mode=_ssgV0EnumToNative(token);
  _ssgReadVec4(fd,specular_colour);_ssgReadVec4(fd,emission_colour);
  _ssgReadVec4(fd,ambient_colour);_ssgReadVec4(fd,diffuse_colour);
  _ssgReadInt(fd,&token);shade_model=(GLenum)_ssgV0EnumToNative(token);
  _ssgReadFloat(fd,&shininess);_ssgReadFloat(fd,&alpha_clamp);_ssgReadInt(fd,&marker);
  ssgBase* loaded=NULL;int needs_load=0;
  if(!_ssgV0ReadObjectHeader(fd,&loaded,ssgTypeTexture(),4,&needs_load))return FALSE;
  if(needs_load==-2){ssgDeRefDelete(texture);texture=NULL;} // Unknown-tag path does not change care bits.
  else setTexture((ssgTexture*)loaded); // Original publishes the texture before loading it.
  if(needs_load>0 && !loaded->load(fd))return FALSE;
  return ssgState::load(fd);
}


int ssgSimpleState::save ( FILE *fd )
{
  // MG-PLIB-019: adapted original version-0 record layout.
  _ssgWriteInt(fd,0x3039);_ssgWriteInt(fd,dont_care);_ssgWriteInt(fd,enables);
  _ssgWriteInt(fd,_ssgV0EnumToWire(colour_material_mode));
  _ssgWriteVec4(fd,specular_colour);_ssgWriteVec4(fd,emission_colour);
  _ssgWriteVec4(fd,ambient_colour);_ssgWriteVec4(fd,diffuse_colour);
  _ssgWriteInt(fd,_ssgV0EnumToWire(shade_model));_ssgWriteFloat(fd,shininess);_ssgWriteFloat(fd,alpha_clamp);
  _ssgWriteInt(fd,0x1e240);if(!_ssgSaveObject(fd,texture))return FALSE;
  return ssgState::save(fd);
}

/*
wk: not implemented yet
Is ssgSimpleState the right place to do this?
use gkGetTexParamiv or so.

int ssgSimpleState::getWrapU()
{
int    wrapu = TRUE, wrapv = TRUE ;

	if ( filename == NULL )
		return TRUE;
	if ( filename[0] == 0 )
		return TRUE;
	GLuint texture_handle = getTextureHandle ();
	
}
*/


static void printStateString ( FILE *fd, unsigned int bits )
{
  if ( bits & (1<<SSG_GL_TEXTURE_EN) ) fprintf ( fd, "TEXTURE2D " ) ;
  if ( bits & (1<<SSG_GL_CULL_FACE_EN) ) fprintf ( fd, "CULLFACE " ) ;
  if ( bits & (1<<SSG_GL_COLOR_MATERIAL_EN) ) fprintf ( fd, "COLOR_MATERIAL ");
  if ( bits & (1<<SSG_GL_BLEND_EN) ) fprintf ( fd, "BLEND " ) ;
  if ( bits & (1<<SSG_GL_ALPHA_TEST_EN) ) fprintf ( fd, "ALPHA_TEST " ) ;
  if ( bits & (1<<SSG_GL_LIGHTING_EN) ) fprintf ( fd, "LIGHTING " ) ;
}

 
void ssgSimpleState::print ( FILE *fd, char *indent, int how_much )
{
  ssgState::print ( fd, indent, how_much ) ;
 
  // MG-PLIB-023/036: original prints all state details unconditionally.

  fprintf ( fd, "%s  Don't Care   = ", indent ) ;
             printStateString ( fd, dont_care ) ;
             fprintf ( fd, "\n" ) ;
  fprintf ( fd, "%s  Enabled      = ", indent ) ;
             printStateString ( fd, enables ) ;
             fprintf ( fd, "\n" ) ;

  fprintf ( fd, "%s  TexHandle    = %d\n", indent, getTextureHandle() ) ;
  fprintf ( fd, "%s  TexFilename  = '%s'\n", indent,
                           (getTextureFilename()==NULL) ? "<none>" : getTextureFilename() ) ;
  fprintf ( fd, "%s  Shade Model  = %d\n", indent, _ssgV0EnumToWire(shade_model) ) ;
  fprintf ( fd, "%s  Shininess    = %f\n", indent, shininess ) ;
  fprintf ( fd, "%s  AlphaClamp   = %f\n", indent, alpha_clamp ) ;
  fprintf ( fd, "%s  ColourMatMode= %s\n", indent,
    (colour_material_mode == GL_AMBIENT) ? "GL_AMBIENT" :
    (colour_material_mode == GL_DIFFUSE) ? "GL_DIFFUSE" :
    (colour_material_mode == GL_AMBIENT_AND_DIFFUSE) ? "GL_AMBIENT_AND_DIFFUSE" :
    (colour_material_mode == GL_SPECULAR) ? "GL_SPECULAR" :
    (colour_material_mode == GL_EMISSION) ? "GL_EMISSION" : "?????" ) ;

  fprintf ( fd, "%s  Ambient  : (%f,%f,%f,%f)\n", indent,
                             ambient_colour[0],  ambient_colour[1],
                             ambient_colour[2],  ambient_colour[3] ) ;
  fprintf ( fd, "%s  Diffuse  : (%f,%f,%f,%f)\n", indent,
                             diffuse_colour[0],  diffuse_colour[1],
                             diffuse_colour[2],  diffuse_colour[3] ) ;
  fprintf ( fd, "%s  Specular : (%f,%f,%f,%f)\n", indent,
                            specular_colour[0], specular_colour[1],
                            specular_colour[2], specular_colour[3] ) ;
  fprintf ( fd, "%s  Emission : (%f,%f,%f,%f)\n", indent,
                            emission_colour[0], emission_colour[1],
                            emission_colour[2], emission_colour[3] ) ;
}

// ***********************************************************************
// ******************** class ssgSimpleStateArray ************************
// ***********************************************************************
 

int ssgSimpleStateArray::findIndex ( ssgSimpleState* st )
{
  for ( int i = 0; i < getNum (); i++ )
    if ( get (i) == st )
      return i ;
  return -1 ;
}


void ssgSimpleStateArray::collect ( ssgEntity *e )
{
  removeAll () ;
	collect_recursive (e);
}


void ssgSimpleStateArray::collect_recursive ( ssgEntity *e )
{
  if ( e -> isAKindOf ( SSG_TYPE_BRANCH ) )
  {
    ssgBranch *br = (ssgBranch *) e ;

    for ( int i = 0 ; i < br -> getNumKids () ; i++ )
      collect_recursive ( br -> getKid ( i ) ) ;
  }
  else
  if ( e -> isAKindOf ( SSG_TYPE_VTXTABLE ) )
  {
    ssgVtxTable* vt = (ssgVtxTable *) e ;
    ssgState* st = vt -> getState () ;
    if ( st && st -> isAKindOf ( SSG_TYPE_SIMPLESTATE ) )
    {
      ssgSimpleState* ss = (ssgSimpleState*) vt -> getState () ;
      if ( findIndex ( ss ) == -1 )
        add ( ss ) ;
    }
  }
}


void ssgSimpleStateArray::add ( ssgSimpleState* ss )
{
  if ( ss )
  {
    ss -> ref () ;
    raw_add ( (char *) &ss ) ;
  }
}


void ssgSimpleStateArray::removeAll ()
{
  for ( int i = 0; i < getNum (); i++ )
    ssgDeRefDelete ( get (i) ) ;
  ssgSimpleList::removeAll () ;
}


ssgSimpleState* ssgSimpleStateArray::findMatch ( ssgSimpleState* st )
{
  if ( st == NULL )
     return NULL ;

  for ( int i = 0; i < getNum (); i++ )
  {
    ssgSimpleState *st2 = get (i) ;

    if ( st == st2 )
      return NULL ; //same pointer -- don't change state

    if ( st->isEnabled ( GL_TEXTURE_2D ) != st2->isEnabled ( GL_TEXTURE_2D ) )
      continue ;

    if ( st->isEnabled ( GL_TEXTURE_2D ) &&
       st -> getTextureHandle () != st2 -> getTextureHandle () )
      continue ;

    if ( st->getCareAbout (SSG_GL_SPECULAR) != st2->getCareAbout (SSG_GL_SPECULAR) ||
      st->getCareAbout (SSG_GL_EMISSION) != st2->getCareAbout (SSG_GL_EMISSION) ||
      st->getCareAbout (SSG_GL_AMBIENT) != st2->getCareAbout (SSG_GL_AMBIENT) ||
      st->getCareAbout (SSG_GL_DIFFUSE) != st2->getCareAbout (SSG_GL_DIFFUSE) )
      continue ;

    if ( ! st->getCareAbout (SSG_GL_SPECULAR) &&
       ! sgEqualVec4 ( st->getMaterial (GL_SPECULAR), st2->getMaterial (GL_SPECULAR) ) )
      continue ;

    if ( ! st->getCareAbout (SSG_GL_EMISSION) &&
       ! sgEqualVec4 ( st->getMaterial (GL_EMISSION), st2->getMaterial (GL_EMISSION) ) )
      continue ;

    if ( ! st->getCareAbout (SSG_GL_AMBIENT) &&
       ! sgEqualVec4 ( st->getMaterial (GL_AMBIENT), st2->getMaterial (GL_AMBIENT) ) )
      continue ;

    if ( ! st->getCareAbout (SSG_GL_DIFFUSE) &&
       ! sgEqualVec4 ( st->getMaterial (GL_DIFFUSE), st2->getMaterial (GL_DIFFUSE) ) )
      continue ;

    if ( st -> isTranslucent () != st2 -> isTranslucent () ||
         st -> getShininess () != st2 -> getShininess () )
      continue ;

    return st2 ;
  }
  return NULL ;
}
