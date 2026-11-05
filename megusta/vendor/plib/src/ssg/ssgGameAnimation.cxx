// MG-PLIB-049: recovered0046d4c0..0046d80d. No upstream identity claim.
#include "ssgGameAnimation.h"
#include "ssgASX.h"
#include <algorithm>
#include <cstdlib>

void ssgAnim::collectFrames(ssgEntity *node,unsigned int& count)
{
  if(node->isAKindOf(SSG_TYPE_BRANCH))
  {
    auto *branch=static_cast<ssgBranch *>(node);
    for(int i=0;i<branch->getNumKids();++i)collectFrames(branch->getKid(i),count);
    if(node->isAKindOf(SSG_TYPE_SELECTOR))
      count=std::max(count,static_cast<unsigned int>(branch->getNumKids()));
    else if(node->isAKindOf(0x1000000))
      count=std::max(count,static_cast<unsigned int>(
        static_cast<megusta::asx::GameAnimatedTransform *>(node)->frameCount()));
  }
  else if(node->isAKindOf(SSG_TYPE_LEAF))
  {
    auto *state=static_cast<ssgLeaf *>(node)->getState();
    if(state && state->isAKindOf(SSG_TYPE_STATESELECTOR))
      count=std::max(count,static_cast<unsigned int>(static_cast<ssgStateSelector *>(state)->getNumSteps()));
  }
}
unsigned int ssgAnim::countFrames(ssgEntity *node)
{
  unsigned int count=0;collectFrames(node,count);return count;
}
bool ssgAnim::isSingleSharedChild(ssgEntity *node)
{
  if(node->getType()!=0x73)return false;
  auto *branch=static_cast<ssgBranch *>(node);
  return branch->getNumKids()==1 && branch->getKid(0)->getNumParents()>1;
}
void ssgAnim::attach(ssgEntity *node,unsigned int count)
{
  entity=node;frame_count=count;current_frame=0;attached=false;
  entity->setUserData(this);
  entity->setTravCallback(SSG_CALLBACK_PRETRAV,traverse);
  attached=true;
}
ssgAnim::ssgAnim(ssgEntity *node,unsigned int count) { attach(node,count); }
ssgAnim::ssgAnim(ssgEntity *node) { attach(node,countFrames(node)); }
void ssgAnim::setFrame(unsigned int frame)
{
  current_frame=frame;
  if(!attached)apply(entity);
}
void ssgAnim::apply(ssgEntity *node)
{
  if(node->isAKindOf(SSG_TYPE_BRANCH))
  {
    auto *branch=static_cast<ssgBranch *>(node);
    for(int i=0;i<branch->getNumKids();++i)apply(branch->getKid(i));
    if(node->isAKindOf(SSG_TYPE_SELECTOR))
    {
      unsigned int count=branch->getNumKids();
      // Empty original selectors underflow and write outside selection storage.
      // Explicit termination is the native malformed-graph adaptation.
      if(!count)std::abort();
      static_cast<ssgSelector *>(node)->selectStep(std::min(current_frame,count-1));
    }
    else if(node->isAKindOf(0x1000000))
      static_cast<megusta::asx::GameAnimatedTransform *>(node)->setFrame(static_cast<int>(current_frame));
  }
  else if(node->isAKindOf(SSG_TYPE_LEAF))
  {
    auto *state=static_cast<ssgLeaf *>(node)->getState();
    if(state && state->isAKindOf(SSG_TYPE_STATESELECTOR))
    {
      auto *selector=static_cast<ssgStateSelector *>(state);
      unsigned int count=selector->getNumSteps();
      selector->selectStep(std::min(current_frame,count-1));
    }
  }
}
int ssgAnim::traverse(ssgEntity *node,int which)
{
  if(which==1)
    if(auto *animation=static_cast<ssgAnim *>(node->getUserData()))animation->apply(node);
  return 1;
}
