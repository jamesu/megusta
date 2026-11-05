#pragma once
// MG-PLIB-076: internal views of the executable's ASE-to-ASX writer inputs.
#include <cstdio>
#include <cstdint>
struct _ssgAsxMaterialView {
  const char* name; uint32_t index,sub_index,sub_flag;
  float ambient[4],diffuse[4],specular[4],shine,transparency;
  const char* texture; float repeat[2],offset[2];
};
struct _ssgAsxMeshView {
  uint32_t faces_count,vertices_count,texcoords_count,colours_count;
  const void* faces;const void* vertices;const void* texcoords;const void* colours;
};
struct _ssgAsxNodeView {
  uint32_t kind;const char* name;const char* parent;uint32_t inherit[3];
  float position[3],target[3];uint32_t material,sample_count;const void* samples;
  const _ssgAsxMeshView* meshes[256];uint32_t mesh_count;
};
// Caller owns the already-open output stream. Null disables conversion.
void _ssgConfigureAseConversion(FILE*,const char* extension,const char* texture_dir,bool embed);
bool _ssgAseConversionEnabled();
void _ssgWriteAseObject(const _ssgAsxMaterialView*,uint32_t,const _ssgAsxNodeView&);
void _ssgFinishAseConversion(const _ssgAsxMaterialView*,uint32_t);
uint32_t _ssgAseConvertedObjectCount();
uint32_t _ssgAseEmbeddedTextureCount();
long _ssgAseConvertedSceneEnd();
