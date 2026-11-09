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

     $Id: pu.cxx,v 1.17 2001/05/16 16:51:40 mcdave Exp $
*/

#include "puLocal.h"
#ifdef MEGUSTA_SDL
#include <SDL.h>
#define timeGetTime SDL_GetTicks
#endif

#if !defined(WIN32) && !defined(MEGUSTA_SDL)
#  ifndef macintosh
#    include <GL/glx.h>
#  else
#    include <agl.h>
#  endif
#endif

#if defined(PU_NOT_USING_GLUT) && !defined(MEGUSTA_PUI_FNT)
#include <iostream>
#endif

#define PU_STRING_X_FUDGE 6
#define PU_STRING_Y_FUDGE 6

int puRefresh = TRUE ;

static int puWindowWidth  = 400 ;
static int puWindowHeight = 400 ;
static int openGLSize = 0 ;

#ifdef MEGUSTA_PUI_FNT
// MG-PLIB-011: native client rectangle replaces the game window provider.
 #if defined(WIN32)
static HWND puNativeWindow = NULL ;
void puSetNativeWindow ( HWND window ) { puNativeWindow = window ; }
int puGetWindow () { return (int)(INT_PTR)puNativeWindow ; }
int puGetWindowHeight () { RECT r ; return !openGLSize && puNativeWindow && GetClientRect(puNativeWindow,&r) ? r.bottom-r.top : puWindowHeight ; }
int puGetWindowWidth () { RECT r ; return !openGLSize && puNativeWindow && GetClientRect(puNativeWindow,&r) ? r.right-r.left : puWindowWidth ; }
#else
static void* puNativeWindow = NULL ;
void puSetNativeWindow ( void* window ) { puNativeWindow = window ; }
int puGetWindow () { return 0 ; }
int puGetWindowHeight () { return puWindowHeight ; }
int puGetWindowWidth () { return puWindowWidth ; }
#endif
void puSetResizeMode ( int mode ) { openGLSize = mode ; }
void puSetWindowSize ( int width, int height ) { puWindowWidth=width ; puWindowHeight=height ; }
#elif defined(PU_NOT_USING_GLUT)

int puGetWindow       () { return 0              ; }
int puGetWindowHeight () { return puWindowHeight ; }
int puGetWindowWidth  () { return puWindowWidth  ; }

void puSetWindowSize ( int width, int height )
{
  puWindowWidth  = width  ;
  puWindowHeight = height ;
}

static int fontBase = 0;
static int fontSize[257];
#else

void puSetResizeMode ( int mode ) {
  openGLSize = mode ;
}


int puGetWindow ()
{
  return glutGetWindow () ;
}


int puGetWindowHeight ()
{
  if ( ! openGLSize )
        return glutGet ( (GLenum) GLUT_WINDOW_HEIGHT ) ;
  else
        return puWindowHeight ;
}


int puGetWindowWidth ()
{
  if ( ! openGLSize )
        return glutGet ( (GLenum) GLUT_WINDOW_WIDTH ) ;
  else
        return puWindowWidth ;
}


void puSetWindowSize ( int width, int height )
{
  if ( ! openGLSize )
    ulSetError ( UL_WARNING, "PUI: puSetWindowSize shouldn't be used with GLUT." ) ;
  else
  {
    puWindowWidth  = width  ;
    puWindowHeight = height ;
  }
}

#endif

puColour _puDefaultColourTable[] =
{
  { 0.5f, 0.5f, 0.5f, 1.0f }, /* PUCOL_FOREGROUND */
  { 0.3f, 0.3f, 0.3f, 1.0f }, /* PUCOL_BACKGROUND */
  { 0.7f, 0.7f, 0.7f, 1.0f }, /* PUCOL_HIGHLIGHT  */
  { 0.0f, 0.0f, 0.0f, 1.0f }, /* PUCOL_LABEL      */
  { 1.0f, 1.0f, 1.0f, 1.0f }, /* PUCOL_TEXT       */

  { 0.0f, 0.0f, 0.0f, 0.0f }  /* ILLEGAL */
} ;
 

static int glIsValidContext ()
{
#if defined(MEGUSTA_PUI_FNT)
  return true ; // MG-PLIB-011: original 00456460 is an unconditional success.
#elif defined(CONSOLE)
  return true ;
#elif defined(WIN32)
  return ( wglGetCurrentContext () != NULL ) ;
#elif defined(macintosh)
  return ( aglGetCurrentContext() != NULL ) ;
#else
  return ( glXGetCurrentContext() != NULL ) ;
#endif
}

puValue::~puValue () {}  

static int _puCursor_enable = FALSE ;
static int _puCursor_x      = 0 ;
static int _puCursor_y      = 0 ;
static float _puCursor_bgcolour [4] = { 1.0f, 1.0f, 1.0f, 1.0f } ; 
static float _puCursor_fgcolour [4] = { 0.0f, 0.0f, 0.0f, 1.0f } ;  

// MG-PLIB-046: executable00456380/00456410/00456420/00456670.
// Names describe recovered roles; the original private spellings are unknown.
static struct puCursorTextureState {
  unsigned int texture;
  int xoffset, yoffset, width, height;
  bool idle_fade;
  float idle_epoch;
  int last_x, last_y;
  bool retain_epoch;
} cursor_texture = {};

void puSetCursorTexture ( unsigned int texture, int xoffset, int yoffset,
                          int width, int height, bool idle_fade )
{
  cursor_texture.texture=texture;cursor_texture.xoffset=xoffset;
  cursor_texture.yoffset=yoffset;cursor_texture.width=width;
  cursor_texture.height=height;cursor_texture.idle_fade=idle_fade;
}
void puResetCursorIdle () { cursor_texture.retain_epoch=true; }

void   puHideCursor ( void ) { _puCursor_enable = FALSE ; }
void   puShowCursor ( void ) { _puCursor_enable = TRUE  ; }
int    puCursorIsHidden ( void ) { return ! _puCursor_enable ; }

void puCursor ( int x, int y )
{
  _puCursor_x = x ;
  _puCursor_y = y ;
  if(cursor_texture.idle_fade)
  {
    if(abs(x-cursor_texture.last_x)<5 && abs(y-cursor_texture.last_y)<5)
      return;
    if(!cursor_texture.retain_epoch)
      cursor_texture.idle_epoch=static_cast<double>(static_cast<int>(timeGetTime()))*0.001f;
    cursor_texture.retain_epoch=false;
    cursor_texture.last_x=x;cursor_texture.last_y=y;
  }
}


void puDrawCursor ( int x, int y )
{
  if(cursor_texture.texture)
  {
    int left=x+cursor_texture.xoffset, top=y+cursor_texture.yoffset;
    int right=left+cursor_texture.width, bottom=top-cursor_texture.height;
    float alpha=1.0f;
    if(cursor_texture.idle_fade)
    {
      double elapsed=(static_cast<double>(static_cast<int>(timeGetTime()))*0.001f
                     -cursor_texture.idle_epoch)-6.0f;
      if(elapsed>0.0)
      {
        double faded=1.0f-elapsed*0.33333334f;
        alpha=static_cast<float>(faded);
        if(faded<0.0)return;
      }
    }
    glEnable(GL_TEXTURE_2D);glEnable(GL_ALPHA_TEST);glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.01f);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,alpha);glBindTexture(GL_TEXTURE_2D,cursor_texture.texture);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex3f(left,top,0);
    glTexCoord2f(1,0);glVertex3f(right,top,0);
    glTexCoord2f(1,1);glVertex3f(right,bottom,0);
    glTexCoord2f(0,1);glVertex3f(left,bottom,0);
    glEnd();
    glDisable(GL_TEXTURE_2D);glDisable(GL_ALPHA_TEST);glDisable(GL_BLEND);
    glAlphaFunc(GL_ALWAYS,0);glBlendFunc(GL_ONE,GL_ZERO);glColor4f(1,1,1,1);
    return;
  }
  glColor4fv ( _puCursor_bgcolour ) ;  

  glBegin    ( GL_TRIANGLES ) ;
  glVertex2i ( x, y ) ;
  glVertex2i ( x + 13, y -  4 ) ;
  glVertex2i ( x +  4, y - 13 ) ;

  glVertex2i ( x +  8, y -  3 ) ;
  glVertex2i ( x + 17, y - 12 ) ;
  glVertex2i ( x + 12, y - 17 ) ;

  glVertex2i ( x + 12, y - 17 ) ;
  glVertex2i ( x +  3, y -  8 ) ;
  glVertex2i ( x +  8, y -  3 ) ;
  glEnd      () ;

  glColor4fv ( _puCursor_fgcolour ) ;  

  glBegin    ( GL_TRIANGLES ) ;
  glVertex2i ( x+1, y-1 ) ;
  glVertex2i ( x + 11, y -  4 ) ;
  glVertex2i ( x +  4, y - 11 ) ;

  glVertex2i ( x +  8, y -  5 ) ;
  glVertex2i ( x + 15, y - 12 ) ;
  glVertex2i ( x + 12, y - 15 ) ;

  glVertex2i ( x + 12, y - 15 ) ;
  glVertex2i ( x +  5, y -  8 ) ;
  glVertex2i ( x +  8, y -  5 ) ;
  glEnd      () ;
}


// Pointer to linked list of objects to delete
// as a result of keyboarding or mouse clicking

static puObject *objects_to_delete = NULL;


void puDeleteObject ( puObject *ob )
{
  puGroup *parent = ob->getParent () ;
  
  // Remove from parent interface
  if ( parent != ob && parent != NULL )
    parent -> remove ( ob ) ;

  // Pop live interface
  if ( ob -> getType () & PUCLASS_INTERFACE )
    puPopLiveInterface ( (puInterface*)ob ) ;

  ob -> prev = NULL ;                       // Add to linked list to be deleted
  ob -> next = objects_to_delete ;
  objects_to_delete = ob ;
  ob -> setParent ( NULL ) ;
}


void puCleanUpJunk ()
{
  /* Step through the linked list of objects to delete, removing them. */
  while ( objects_to_delete != NULL )
  {
    puObject *next_ob = objects_to_delete ->next ;
    delete objects_to_delete ;
    objects_to_delete = next_ob ;
  }
}

void puShutdown ()
{
  puCleanUpJunk();
  delete puGetUltimateLiveInterface();
}


static puObject *active_widget ;   /* Widget which is currently receiving user input */
static char *input_paste_buffer ;  /* Cut/Copy/Paste buffer for input widgets */

void puInit ( void )
{
  static int firsttime = TRUE ;

  if ( firsttime )
  {
    if ( glIsValidContext () == 0 )
    {
      ulSetError ( UL_FATAL,
        "puInit called without a valid OpenGL context.");
    }

    puInterface *base_interface = new puInterface ( 0, 0 ) ;
    puPushGroup         ( base_interface ) ;
    puPushLiveInterface ( base_interface ) ;

    active_widget = NULL ;
    input_paste_buffer = NULL ;

    firsttime = FALSE ;
#if defined(PU_NOT_USING_GLUT) && !defined(MEGUSTA_PUI_FNT)

    /* Create bitmaps for the device context font's first 256 glyphs */

    fontBase = glGenLists(256);
    assert(fontBase);
    HDC hdc = wglGetCurrentDC();

    /* Make the system font the device context's selected font */

    SelectObject (hdc, GetStockObject (SYSTEM_FONT)); 

    int *tempSize = &fontSize[1];

    if ( ! GetCharWidth32 ( hdc, 1, 255, tempSize ) &&
         ! GetCharWidth   ( hdc, 1, 255, tempSize ) )
    {
      LPVOID lpMsgBuf ;

      FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0, NULL ) ;

      ulSetError ( UL_WARNING, "PUI: Error: %s", (char *)lpMsgBuf ) ;
      LocalFree ( lpMsgBuf ) ;
    }

    wglUseFontBitmaps ( hdc, 0, 256, fontBase ) ;
#endif
  }
}

// MG-PLIB-030: 00456970 leaves projection/model-view setup to the game.
static void puSetOpenGLState ( void )
{
  glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
}
static void puRestoreOpenGLState ( void ) {} // Original restore entry is a no-op.
int _puDrawMinPriority=-99999, _puDrawMaxPriority=99999;
void puDisplay ( int minimum_priority, int maximum_priority )
{
  _puDrawMinPriority=minimum_priority;_puDrawMaxPriority=maximum_priority;
  puCleanUpJunk();puSetOpenGLState();
  puGetUltimateLiveInterface()->draw(0,0);
  int height=puGetWindowHeight();
  if(_puCursor_enable)puDrawCursor(_puCursor_x,height-_puCursor_y);
  puRestoreOpenGLState();
  _puDrawMinPriority=-99999;_puDrawMaxPriority=99999;
}
void puDisplay () { puDisplay(-99999,99999); }


int puKeyboard ( int key, int updown )
{
  int return_value = puGetBaseLiveInterface () -> checkKey ( key, updown ) ;
  
  puCleanUpJunk () ;
  
  return return_value ;
}


static int last_buttons = 0 ;
int puMouse ( int button, int updown, int x, int y )
{
  puCursor ( x, y ) ;
  
  int h = puGetWindowHeight () ;
  
  if ( updown == PU_DOWN )
    last_buttons |=  ( 1u << (static_cast<unsigned int>(button)&31) ) ;
  else
    last_buttons &= ~( 1u << (static_cast<unsigned int>(button)&31) ) ;
  
  int return_value =  puGetBaseLiveInterface () -> checkHit ( button,
    updown, x, h - y ) ;
  
  puCleanUpJunk () ;
  
  return return_value ;
}


int puMouse ( int x, int y )
{
  puCursor ( x, y ) ;
  
  if ( last_buttons == 0 )
    return FALSE ;
  
  int button =
    (last_buttons & (1<<PU_LEFT_BUTTON  )) ?  PU_LEFT_BUTTON   :
    (last_buttons & (1<<PU_MIDDLE_BUTTON)) ?  PU_MIDDLE_BUTTON :
    (last_buttons & (1<<PU_RIGHT_BUTTON )) ?  PU_RIGHT_BUTTON  : 0 ;
  
  int h = puGetWindowHeight () ;
  
  int return_value = puGetBaseLiveInterface () -> checkHit ( button,
    PU_DRAG, x, h - y ) ;
  
  puCleanUpJunk () ;
  
  return return_value ;
}

void puMoveToLast (puObject *ob)
{
  puGroup *parent = ob -> getParent () ;

  /* If no parent interface, return. */

  if ( ! parent ) return;

  /* Remove "ob" from present place in the "dlist" list */

  parent -> remove (ob) ;

  /* Place at the end of the list */

  parent -> add (ob) ;

  /*
    Now repeat the process for the parent interface so that the interface will
    be drawn last of all interfaces.
  */

  puMoveToLast ( parent );
}

void puDeactivateWidget ()  {  active_widget = NULL ; }
void puSetActiveWidget ( puObject *w ) {   active_widget = w ; }
puObject *puActiveWidget () {   return active_widget ; }

void puSetPasteBuffer ( char *ch )
{
  if ( input_paste_buffer ) delete input_paste_buffer ;
  input_paste_buffer = new char [ strlen(ch) + 1 ] ;
  strcpy ( input_paste_buffer, ch ) ;
}

char *puGetPasteBuffer ()  {  return input_paste_buffer ;  }

