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

     $Id: ssgLoadSSG.cxx,v 1.16 2001/04/12 03:38:15 deca Exp $
*/

// MG-PLIB-019: adapted executable version-0 serialization, not the 1.3.2 wire format.
#include "ssgLocal.h"
#include "megusta_v0_tokens.h"
#include <vector>

int _ssgFileVersionNumber=0;
static int next_key=0;
static std::vector<ssgBase*> instances;
void _ssgV0ResetReadContext() { instances.clear();next_key=0; }
int _ssgV0NextKey() { return ++next_key; }
void _ssgV0Register(int key,ssgBase* object) {
  if(key<0){ulSetError(UL_WARNING,"SSG: invalid instance key");return;} // Avoid original negative-index memory corruption.
  if(key==0)ulSetError(UL_WARNING,"Key zero encountered while reading SSG");
  if(size_t(key)>=instances.size()) {
    size_t capacity=instances.size()*2;
    if(capacity<=size_t(key))capacity=size_t(key)+128;
    if(key==0 && instances.empty())capacity=10;
    instances.resize(capacity,NULL);
  }
  instances[key]=object;
}
static ssgBase* lookup(int key) {
  if(key<0 || size_t(key)>=instances.size()){ulSetError(UL_WARNING,"Invalid key encountered while reading SSG");return NULL;}
  return instances[key];
}
unsigned int _ssgV0EnumToNative(unsigned int token) { return token<128 ? ssg_v0_graphics_tokens[token] : token; }
unsigned int _ssgV0EnumToWire(unsigned int value) {
  for(unsigned int i=0;i<128;++i)if(ssg_v0_graphics_tokens[i]==value)return i;
  return value; // Preserve opaque values outside the evidenced table.
}
int _ssgV0Type(ssgBase* object) {
  int type=object->getType();
  if(type==ssgTypeVtxArray())return 0xc007;
  if(type==ssgTypeVtxTable())return 0x4007;
  if(type==ssgTypeRoot())return 0x413;
  if(type==ssgTypeTransform())return 0x73;
  if(type==ssgTypeTexTrans())return 0x2013;
  if(type==ssgTypeSelector())return 0x113;
  if(type==ssgTypeCutout())return 0x813;
  if(type==ssgTypeBranch())return 0x13;
  if(type==ssgTypeStateSelector())return 0x1d;
  if(type==ssgTypeSimpleState())return 0xd;
  if(type==ssgTypeTexture())return 0x20001;
  return type; // MG-PLIB-039: original writer emits even unknown runtime tags.
}
static ssgBase* create(int tag,int flags) {
  switch(tag) {
    case 0x4007:return new ssgVtxTable;
    case 0xc007:return new ssgVtxArray;
    case 0x13:return new ssgBranch;
    case 0x73:return new ssgTransform;
    case 0x113:return new ssgSelector(32);
    case 0x813:return new ssgCutout(0);
    case 0x2013:return new ssgTexTrans;
    case 0x413:return new ssgRoot;
    case 0xd:return new ssgSimpleState;
    case 0x1d:if(flags&2)return new ssgSimpleState;else return new ssgStateSelector;
    case 0x20001:return new ssgTexture;
    default:return NULL;
  }
}
int _ssgV0ReadObjectHeader(FILE* file,ssgBase** result,int mask,int flags,int* load_needed) {
  int tag=0,key=0;*load_needed=0;_ssgReadInt(file,&tag);if(_ssgReadError())return FALSE;
  if(tag==0) {
    _ssgReadInt(file,&key);if(_ssgReadError())return FALSE;
    *result=key ? lookup(key) : NULL;
    if(!*result)return (flags&4) || (key==0 && !(flags&1));
    if(flags&1)*load_needed=-1; // Existing branch references are loaded again.
    return TRUE;
  }
  bool state_tag=tag==0xd || tag==0x1d;
  bool texture_tag=tag==0x20001;
  if((mask==ssgTypeState() && !state_tag) || (mask==ssgTypeTexture() && !texture_tag)) {
    ulSetError(UL_WARNING,"loadSSG: unrecognised field object type 0x%08x",tag);
    *result=NULL;*load_needed=-2;return (flags&4)!=0; // Unknown field, not a null-reference setter call.
  }
  ssgBase* object=create(tag,flags);
  if(!object || (mask && !object->isAKindOf(mask))) {
    delete object;ulSetError(UL_WARNING,"loadSSG: Unrecognised object type 0x%08x",tag);return FALSE;
  }
  *result=object;*load_needed=1;return TRUE;
}
int _ssgLoadObjectV0(FILE* file,ssgBase** result,int mask,int flags) {
  ssgBase* object=NULL;int load_needed=0;
  if(!_ssgV0ReadObjectHeader(file,&object,mask,flags,&load_needed))return FALSE;
  if(load_needed && load_needed!=-2 && !object->load(file)) {
    if(load_needed==1)delete object;
    return FALSE;
  }
  *result=object;return TRUE;
}
int _ssgLoadObject(FILE* file,ssgBase** result,int mask) { return _ssgLoadObjectV0(file,result,mask,0); }
int _ssgSaveObject(FILE* file,ssgBase* object) {
  if(!object || object->getSpare()>0) {
    _ssgWriteInt(file,0);_ssgWriteInt(file,object ? object->getSpare() : 0);return !_ssgWriteError();
  }
  int tag=_ssgV0Type(object);
  _ssgWriteInt(file,tag);return object->save(file);
}
ssgEntity* ssgLoadSSG(const char* filename,const ssgLoaderOptions* options) {
  ssgSetCurrentOptions((ssgLoaderOptions*)options);
  char path[1024];ssgGetCurrentOptions()->makeModelPath(path,filename);
  _ssgV0ResetReadContext();
  FILE* file=fopen(path,"rb");if(!file){perror(path);ulSetError(UL_WARNING,"ssgLoadSSG: Failed to open '%s' for reading\n",path);return NULL;}
  int magic=0;_ssgReadInt(file,&magic);
  if((unsigned int)magic>>8!=0x535347u) {
    if(((unsigned int)magic & 0xffffffu)==0x475353u)
      ulSetError(UL_WARNING,"ssgLoadSSG: File appears to be byte swapped!");
    else ulSetError(UL_WARNING,"ssgLoadSSG: File has incorrect magic number!");
    fclose(file);return NULL;
  }
  if((magic&255)!=0){
    ulSetError(UL_WARNING,"ssgLoadSSG: This version of SSG is too old to load this file!");
    fclose(file);return NULL;
  }
  int previous=_ssgFileVersionNumber;_ssgFileVersionNumber=0;
  // MG-PLIB-053: top-level reader accepts concrete entity tags only, not
  // the generic field-object null/reference/state grammar.
  int tag=0;_ssgReadInt(file,&tag);
  ssgBase* object=NULL;
  switch(tag) {
    case 0x4007:case 0xc007:case 0x13:case 0x73:
    case 0x113:case 0x813:case 0x2013:case 0x413:object=create(tag,0);break;
    default:ulSetError(UL_WARNING,"loadSSG: Unrecognised Entity type 0x%08x\n",tag);
  }
  int okay=object && object->load(file);
  if(object && !okay) {
    ulSetError(UL_WARNING,"loadSSG: Failed to read child object.\n");
    delete object;object=NULL; // Target cleanup replaces original leaked object.
  }
  if(okay && object)((ssgEntity*)object)->recalcBSphere();
  fclose(file);_ssgFileVersionNumber=previous;
  return okay ? (ssgEntity*)object : NULL;
}
int ssgSaveSSG(const char* filename,ssgEntity* object) {
  if(!object)return FALSE;
  next_key=0;object->zeroSpareRecursive();
  char path[1024];ssgGetCurrentOptions()->makeModelPath(path,filename);
  FILE* file=fopen(path,"wb");if(!file){perror(path);ulSetError(UL_WARNING,"ssgSaveSSG: Failed to open '%s' for writing\n",path);return FALSE;}
  _ssgWriteInt(file,0x53534700);
  _ssgWriteInt(file,_ssgV0Type(object));
  int okay=object->save(file);
  if(!okay)ulSetError(UL_WARNING,"ssgSaveSSG: Failed to write child object.\n");
  fclose(file);return okay;
}
