// MG-PLIB-049: executable-derived SSG extension, not upstream PLIB source.
// ssgAnim is the recovered binding class name; native layout is independent.
#ifndef MEGUSTA_SSG_GAME_ANIMATION_H
#define MEGUSTA_SSG_GAME_ANIMATION_H
#include "ssg.h"
class ssgAnim : public ssgBase {
  ssgEntity *entity;
  unsigned int frame_count, current_frame;
  bool attached;
  static void collectFrames(ssgEntity *,unsigned int&);
  static int traverse(ssgEntity *,int);
  void attach(ssgEntity *,unsigned int);
  void apply(ssgEntity *);
public:
  explicit ssgAnim(ssgEntity *);
  ssgAnim(ssgEntity *,unsigned int);
  void setFrame(unsigned int);
  unsigned int getFrame() const { return current_frame; }
  unsigned int getNumFrames() const { return frame_count; }
  static unsigned int countFrames(ssgEntity *);
  static bool isSingleSharedChild(ssgEntity *);
};
#endif
