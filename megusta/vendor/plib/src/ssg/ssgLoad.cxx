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

     $Id: ssgLoad.cxx,v 1.17 2001/05/06 15:30:38 wolfram_kuss Exp $
*/


#include "ssgLocal.h" 
#include "ssgASX.h"
#include <cstdlib>

// MG-PLIB-052: original globals0053dde0/0053dde4 and0053ddf8.
char *ssgLoaderOptions::model_dir=NULL;
char *ssgLoaderOptions::texture_dir=NULL;
ssgSimpleStateArray ssgLoaderOptions::shared_states(3);
ssgTextureArray ssgLoaderOptions::shared_textures;
// Target compiler cleanup replaces the two original atexit path releases.
static struct LoaderPathCleanup {
  ~LoaderPathCleanup() {
    ssgLoaderOptions::releaseModelPath();
    ssgLoaderOptions::releaseTexturePath();
  }
} loader_path_cleanup;
void ssgLoaderOptions::releaseModelPath() { delete [] model_dir;model_dir=NULL; }
void ssgLoaderOptions::releaseTexturePath() { delete [] texture_dir;texture_dir=NULL; }


static ssgLoaderOptions default_options ;

ssgLoaderOptions *_ssgCurrentOptions = &default_options ;



char* ssgLoaderOptions::make_path ( char* path,
      const char* dir, const char* fname ) const
{
  if ( fname != NULL && fname [ 0 ] != '\0' )
  {
    if ( fname [ 0 ] != '/' &&
       dir != NULL && dir[0] != '\0' )
    {
      strcpy ( path, dir ) ;
      strcat ( path, "/" ) ;
      strcat ( path, fname ) ;
    }
    else
      strcpy ( path, fname ) ;

    //convert backward slashes to forward slashes
    for ( char* ptr = path ; *ptr ; ptr ++ )
    {
      if ( *ptr == '\\' )
        *ptr = '/' ;
    }
  }
  else
     path [0] = 0 ;
  return( path );
}


void ssgLoaderOptions::makeModelPath ( char *path, const char *fname ) const
{
  make_path ( path, model_dir, fname ) ;
}


void ssgLoaderOptions::makeTexturePath ( char *path, const char *fname ) const
{
  // MG-PLIB-040: original join preserves relative subdirectories.
  make_path(path,texture_dir,fname);
}


ssgLeaf* ssgLoaderOptions::createLeaf ( ssgLeaf* leaf,
                                        const char* parent_name )
{
  if ( leaf != NULL )
  {
    /* try to do some state sharing */
    ssgState* st = leaf -> getState () ;
    // MG-PLIB-085: 0045acb0 shares only the exact simple-state type.
    if ( st != NULL && st -> isA ( ssgTypeSimpleState () ) )
    {
      ssgSimpleState *ss = (ssgSimpleState*) st ;
      ssgSimpleState *match = shared_states.findMatch ( ss ) ;
      if ( match != NULL )
        leaf -> setState ( match ) ;
      else
        shared_states.add ( ss ) ;
    }
    // The executable compiles the leaf after its final state is selected.
    // This uses the same display-list implementation as ssgLeaf::makeDList.
#ifdef _SSG_USE_DLIST
    leaf -> makeDList () ;
#endif
  }
  return leaf ;
}


ssgTexture* ssgLoaderOptions::createTexture ( char* tfname,
						     int wrapu,
						     int wrapv,
						     int mipmap ) 
{
  
  // MG-PLIB-034: 0045ad20 looks up the caller's original filename globally.
  // Wrap/mipmap flags do not participate in the cache key; no cache-owned ref.
  ssgTexture *tex = ssgTexture::findByFilename ( tfname ) ;
  if ( tex )
    return tex ;
  
  tex = new ssgTexture ( tfname, wrapu, wrapv, mipmap ) ;
  return tex ;
}

ssgTransform* ssgLoaderOptions::createTransform ( ssgTransform* tr,
      ssgTransformArray* ta ) const
{
  // MG-PLIB-081: 0045ada0 replaces the original transform when frames exist.
  if ( ta == NULL ) return tr ;
  delete tr ;
  return new megusta::asx::GameAnimatedTransform(ta) ;
}


void ssgLoaderOptions::setModelDir ( const char *s )
{
  delete [] model_dir ; // MG-PLIB-004/052: global array ownership.
  model_dir = new char [ strlen ( s ) + 1 ] ;
  strcpy ( model_dir, s ) ;
}

void ssgLoaderOptions::setTextureDir ( const char *s )
{
  delete [] texture_dir ;
  texture_dir = new char [ strlen ( s ) + 1 ] ;
  strcpy ( texture_dir, s ) ;
}


static const char *file_extension ( const char *fname )
{
  const char *p = & ( fname [ strlen(fname) ] ) ;

  while ( p != fname && *p != '/' && *p != '.' )
    p-- ;

  return p ;
}


struct _ssgModelFormat
{
  const char *extension ;
  ssgLoadFunc *loadfunc ;
  ssgSaveFunc *savefunc ;
} ;


enum { MAX_FORMATS = 100 } ;

// MG-PLIB-047: fixed original table00506c40, in its original search order.
// The ASX adapter retains the explicit unsupported-path errors of the reader.
static ssgEntity *load_asx(const char *filename,const ssgLoaderOptions *options)
{
  ssgSetCurrentOptions(const_cast<ssgLoaderOptions *>(options));
  auto scene=megusta::asx::load_asx_scene(filename);
  return scene.root_owner.release();
}
// The saved writer entry routes unsupported saves through exit(1).
static int save_asx(const char *,ssgEntity *) { std::exit(1); return 0; }
static _ssgModelFormat formats [ MAX_FORMATS ] = {
  {".ase",ssgLoadASE,ssgSaveASE},
  {".ssg",ssgLoadSSG,ssgSaveSSG},
  {".asx",load_asx,save_asx}
};
static int num_formats = 3 ;


void ssgAddModelFormat ( const char* extension,
                        ssgLoadFunc *loadfunc , ssgSaveFunc  *savefunc )
{
  if ( num_formats < MAX_FORMATS )
  {
    formats [ num_formats ] . extension = extension ;
    formats [ num_formats ] . loadfunc = loadfunc ;
    formats [ num_formats ] . savefunc = savefunc ;
    num_formats ++ ;
  }
  else
  {
    ulSetError ( UL_WARNING, "ssgAddModelFormat: too many formats" );
  }
}



ssgEntity *ssgLoad ( const char *fname, const ssgLoaderOptions* options )
{
  if ( fname == NULL || *fname == '\0' )
    return NULL ;

	// find appropiate loader and call its loadfunc
  const char *extn = file_extension ( fname ) ;
  if ( *extn != '.' )
  {
    ulSetError ( UL_WARNING, "ssgLoad: Cannot determine file type for '%s'", fname );
    return NULL ;
  }

  _ssgModelFormat *f = formats ;
  for ( int i=0; i<num_formats; i++, f++ )
  {
    if ( f->loadfunc != NULL &&
	       ulStrNEqual ( extn, f->extension, strlen(f->extension) ) )
    {
      ssgEntity* entity = f -> loadfunc( fname, options ) ;
      return entity ;
    }
  }

  ulSetError ( UL_WARNING, "ssgLoad: Unrecognised file type '%s'", extn ) ;
  return NULL ;
}


int ssgSave ( const char *fname, ssgEntity *ent )
{
  if ( fname == NULL || ent == NULL || *fname == '\0' )
    return FALSE ;

  const char *extn = file_extension ( fname ) ;

  if ( *extn != '.' )
  {
    ulSetError ( UL_WARNING, "ssgSave: Cannot determine file type for '%s'", fname );
    return FALSE ;
  }

  _ssgModelFormat *f = formats ;
  for ( int i=0; i<num_formats; i++, f++ )
  {
    if ( f->savefunc != NULL &&
         ulStrEqual ( extn, f->extension ) )
      return f->savefunc( fname, ent ) ;
  }

  ulSetError ( UL_WARNING, "ssgSave: Unrecognised file type '%s'", extn ) ;
  return FALSE ;
}
