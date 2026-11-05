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

     $Id: puInput.cxx,v 1.8 2001/05/16 16:51:40 mcdave Exp $
*/


#include "puLocal.h"

void puInput::normalize_cursors ( void )
{
  char val [ PUSTRING_MAX ] ;
  getValue ( val ) ;
  int sl = strlen ( val ) ;

  /* Clamp the positions to the limits of the text.  */

  if ( cursor_position       <  0 ) cursor_position       =  0 ;
  if ( select_start_position <  0 ) select_start_position =  0 ;
  if ( select_end_position   <  0 ) select_end_position   =  0 ;
  if ( cursor_position       > sl ) cursor_position       = sl ;
  if ( select_start_position > sl ) select_start_position = sl ;
  if ( select_end_position   > sl ) select_end_position   = sl ;

  /* Swap the ends of the select window if they get crossed over */

  if ( select_end_position < select_start_position )
  {
    int tmp = select_end_position ;     
    select_end_position = select_start_position ;     
    select_start_position = tmp ;
  }
}

void puInput::removeSelectRegion ( void )
{
  char *p1 = getStringValue () + select_start_position ;
  char *p2 = getStringValue () + select_end_position   ;

  while ( *p1 != '\0' )
    *p1++ = *p2++ ;

  *p2 = '\0' ;

  cursor_position = select_start_position ;
  select_end_position = select_start_position ;
}


void puInput::draw ( int dx, int dy )
{
  normalize_cursors () ;

  if ( !visible || ( window != puGetWindow () ) ) return ;

  /* 3D Input boxes look nicest if they are always in inverse style. */

  abox . draw ( dx, dy, ( (style==PUSTYLE_SMALL_BEVELLED ||
                           style==PUSTYLE_SMALL_SHADED) ) ? -style :
                        (accepting ? -style : style ), colour, FALSE ) ;

  int xx = puGetStringWidth ( legendFont, " " ) ;
  int yy = ( abox.max[1] - abox.min[1] - puGetStringHeight(legendFont) ) / 2 ;

  if ( accepting )
  {
    char val [ PUSTRING_MAX ] ;
    getValue ( val ) ;

    /* Highlight the select area */

    if ( select_end_position > 0 &&
         select_end_position != select_start_position )    
    {
      val [ select_end_position ] = '\0' ;
      int cpos2 = puGetStringWidth ( legendFont, val ) + xx + dx + abox.min[0] ;
      val [ select_start_position ] = '\0' ;
      int cpos1 = puGetStringWidth ( legendFont, val ) + xx + dx + abox.min[0] ;

      glColor3f ( 1.0f, 1.0f, 0.7f ) ;
      // MG-PLIB-057: original emits a polygon in this vertex order.
      glBegin(GL_POLYGON);
      glVertex2i(cpos1,dy+abox.min[1]+6);glVertex2i(cpos2,dy+abox.min[1]+6);
      glVertex2i(cpos2,dy+abox.max[1]-6);glVertex2i(cpos1,dy+abox.max[1]-6);
      glEnd();
    }
  }

  /* Draw the text */

  {
    /* If greyed out then halve the opacity when drawing the label and legend */

    if ( active )
      glColor4fv ( colour [ PUCOL_LEGEND ] ) ;
    else
      glColor4f ( colour [ PUCOL_LEGEND ][0],
                  colour [ PUCOL_LEGEND ][1],
                  colour [ PUCOL_LEGEND ][2],
                  colour [ PUCOL_LEGEND ][3] / 2.0f ) ; /* 50% more transparent */

    char val [ PUSTRING_MAX ] ;
    getValue ( val ) ;

    puDrawString ( legendFont, val,
                  dx + abox.min[0] + xx,
                  dy + abox.min[1] + yy ) ;

    draw_label ( dx, dy ) ;
  }

  if ( accepting )
  { 
    char val [ PUSTRING_MAX ] ;
    getValue ( val ) ;

    /* Draw the 'I' bar cursor. */

    if ( cursor_position >= 0 )
    {
      val [ cursor_position ] = '\0' ;

      int cpos = puGetStringWidth ( legendFont, val ) + xx + dx + abox.min[0] ;
      int top = abox.max[1] - abox.min[1] - 7 ;
      int bot = 7 ; // MG-PLIB-057: original fixed seven-pixel inset.

      glColor3f ( 0.1f, 0.1f, 1.0f ) ;
      glBegin   ( GL_LINES ) ;
      glVertex2i ( cpos    , dy + abox.min[1] + bot ) ;
      glVertex2i ( cpos    , dy + abox.min[1] + top ) ;
      glVertex2i ( cpos - 1, dy + abox.min[1] + bot ) ;
      glVertex2i ( cpos - 1, dy + abox.min[1] + top ) ;
      glVertex2i ( cpos - 4, dy + abox.min[1] + bot ) ;
      glVertex2i ( cpos + 3, dy + abox.min[1] + bot ) ;
      glVertex2i ( cpos - 4, dy + abox.min[1] + top ) ;
      glVertex2i ( cpos + 3, dy + abox.min[1] + top ) ;
      glEnd      () ;
    }
  }
}


// MG-PLIB-057: executable0046a250 and0046a3e0 input event behavior.
void puInput::doHit ( int button, int updown, int x, int )
{
  puObject *active = puActiveWidget();
  if (active && active != this) {
    puCallback callback = active->getDownCallback();
    if (callback) (*callback)(active);
    puDeactivateWidget();
  }
  if(button!=PU_LEFT_BUTTON) { lowlight();return; }
  if(updown!=active_mouse_edge && active_mouse_edge!=PU_UP_AND_DOWN) {
    highlight();return;
  }
  lowlight();
  char value[PUSTRING_MAX];getValue(value);
  int position=strlen(value);
  int width=puGetStringWidth(legendFont,value)+abox.min[0];
  // The binary decrements below zero for clicks left of the text. Clamp this
  // malformed-coordinate edge instead of reproducing its out-of-bounds write.
  while(x<=width && position>0) {
    value[--position]=0;
    width=puGetStringWidth(legendFont,value)+abox.min[0];
  }
  if(x<=width && position==0)position=-1;
  accepting=TRUE;cursor_position=position;normalize_cursors();
  invokeCallback();puSetActiveWidget(this);
}

int puInput::checkKey ( int key, int )
{
  if(!isAcceptingInput() || !isActive() || !isVisible() || window!=puGetWindow())return FALSE;
  puObject *active = puActiveWidget();
  if(active && active != this) {
    puCallback callback = active->getDownCallback();
    if(callback) (*callback)(active);
    puDeactivateWidget();
  }
  normalize_cursors();
  char *value=getStringValue();
  switch(key) {
    case PU_KEY_PAGE_UP:case PU_KEY_PAGE_DOWN:case PU_KEY_INSERT:return FALSE;
    case PU_KEY_UP:case PU_KEY_DOWN:case 27:case '\t':case '\r':case '\n':
      rejectInput();normalize_cursors();invokeCallback();puDeactivateWidget();break;
    case '\b':
      if(cursor_position>0) { --cursor_position;char *p=value+cursor_position;
        while(*p) { *p=*(p+1);++p; } }
      break;
    case 0x7f:
      if(select_start_position==select_end_position) {
        char *p=value+cursor_position;while(*p) { *p=*(p+1);++p; }
      } else {
        // Preserve cursor, while avoiding the original copying beyond NUL.
        char *begin=value+select_start_position,*end=value+select_end_position;
        memmove(begin,end,strlen(end)+1);select_end_position=select_start_position;
      }
      break;
    case 0x15:value[0]=0;break;
    case PU_KEY_HOME:cursor_position=0;break;
    case PU_KEY_END:cursor_position=PUSTRING_MAX;break;
    case PU_KEY_LEFT:--cursor_position;break;
    case PU_KEY_RIGHT:++cursor_position;break;
    default:
      if(key<' ' || key>127 || strlen(value)>=PUSTRING_MAX-1)return FALSE;
      // MG-PLIB-063: original inserts into the internal text, then re-reads
      // the bound string on synchronization. A bound external value therefore
      // replaces the insertion, although the cursor still advances.
      { size_t len=strlen(string);memmove(string+cursor_position+1,string+cursor_position,len-cursor_position+1);
        string[cursor_position]=char(key);++cursor_position; }
  }
  setValue(getStringValue());normalize_cursors();return TRUE;
}
