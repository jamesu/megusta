// MG-PLIB-038: faithful reimplementation from MINIGOLF.EXE 0047e1f0..0047e4d0.
// No upstream source identity is claimed for this game-specific SSG derivative.
#include "ssgLocal.h"
#include <cstdlib>

ssgCompiledLeaf::ssgCompiledLeaf(ssgLeaf* source)
{
  setCullFace(source->getCullFace());
  setRenderOrderMode(source->getRenderOrderMode());
  setState(source->getState());
  compiled_vertices=source->cached_vertices_count;
  compiled_triangles=source->getNumTriangles();
  emptyBSphere();
  extendBSphere(source->getBSphere());
  recalcBSphere();
  dlist=source->dlist;
  source->dlist=0;
  if(!dlist)std::abort();
}

void ssgCompiledLeaf::recalcBSphere()
{
  dirtyBSphere();
  bsphere_is_invalid=FALSE;
}

void ssgCompiledLeaf::draw()
{
  if(!preDraw())return;
  if(hasState())getState()->apply();
  ++stats_num_leaves;
  stats_num_triangles+=compiled_triangles;
  stats_num_vertices+=compiled_vertices;
  if(!dlist)std::abort();
  glCallList(dlist);
  if(postDrawCB)postDrawCB(this);
}

float* ssgCompiledLeaf::getVertex(int){std::abort();}
float* ssgCompiledLeaf::getNormal(int){std::abort();}
float* ssgCompiledLeaf::getColour(int){std::abort();}
float* ssgCompiledLeaf::getTexCoord(int){std::abort();}
void ssgCompiledLeaf::setPrimitiveType(GLenum){std::abort();}
GLenum ssgCompiledLeaf::getPrimitiveType(){std::abort();}
void ssgCompiledLeaf::getTriangle(int,short*,short*,short*){std::abort();}
void ssgCompiledLeaf::transform(const sgMat4){std::abort();}
void ssgCompiledLeaf::print(FILE*,char*,int){std::abort();}
int ssgCompiledLeaf::load(FILE*){std::abort();}
int ssgCompiledLeaf::save(FILE*){std::abort();}
