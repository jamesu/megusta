// MG-PLIB-041: game material-state policy from original00464dd0.
// This differs from PLIB1.3.2's material-lighting policy.
#include "ssgLocal.h"
#include "megusta_ase_support.h"
#include <cmath>
bool _ssgUseMaterialTransparency=true; // Original0050781c starts at1.
bool _ssgAseCameraEndpoint(const float* position,const float* target,float* endpoint)
{
  float d[3];for(int i=0;i<3;++i)d[i]=target[i]-position[i];
  // 00463bf9 computes z²+y²+x² in x87, then rounds before sqrt.
  volatile float square=(long double)d[2]*d[2]+(long double)d[1]*d[1]+(long double)d[0]*d[0];
  if(std::sqrt((long double)square)==0.0L)return false;
  square=(float)sgScalarProductVec3Retained(d,d);
  long double inv=1.0L/std::sqrt((long double)square);
  volatile float x=(float)(inv*d[0]),y=(float)(inv*d[1]);
  endpoint[0]=x+position[0];endpoint[1]=y+position[1];
  endpoint[2]=(float)(inv*d[2]+position[2]);
  return true;
}
ssgSimpleState* _ssgBuildAseMaterialState(const char* filename,int material_index,
    float transparency,ssgLoaderOptions* options)
{
  if(!options)options=ssgGetCurrentOptions();
  if(filename){
    ssgSimpleState* custom=options->createSimpleState(const_cast<char*>(filename),material_index);
    if(custom)return custom;
  }
  ssgSimpleState* state=new ssgSimpleState;
  bool alpha=_ssgUseMaterialTransparency && double(transparency)>0.01;
  if(!filename)state->disable(GL_TEXTURE_2D);
  else{
    ssgTexture* texture=options->createTexture(const_cast<char*>(filename),1,1,1);
    if(!alpha)alpha=texture->hasAlpha();
    state->setTexture(texture);state->enable(GL_TEXTURE_2D);
  }
  state->disable(GL_ALPHA_TEST);
  if(alpha){state->enable(GL_BLEND);state->setTranslucent();}
  else{state->disable(GL_BLEND);state->setOpaque();}
  state->disable(GL_LIGHTING);
  state->setShadeModel(GL_SMOOTH);
  return state;
}
