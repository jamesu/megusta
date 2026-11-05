#pragma once
class ssgSimpleState;
class ssgLoaderOptions;
extern bool _ssgUseMaterialTransparency;
// MG-PLIB-077: returns false for a zero-length camera direction.
bool _ssgAseCameraEndpoint(const float* position,const float* target,float* endpoint);
ssgSimpleState* _ssgBuildAseMaterialState(const char* filename,int material_index,
    float transparency,ssgLoaderOptions* options=nullptr);
// MG-PLIB-041: original00464f20 retains 1-v across the first float store,
// then rounds each multiply and offset addition at the observed stores.
inline void _ssgAseTexCoord(float* uv,const float* repeat,const float* offset)
{
  double flipped=1.0-double(uv[1]);
  uv[1]=float(flipped);
  uv[0]=float(double(repeat[0])*uv[0]);
  uv[1]=float(flipped*repeat[1]);
  uv[0]=float(double(offset[0])+uv[0]);
  uv[1]=float(double(offset[1])+uv[1]);
}
