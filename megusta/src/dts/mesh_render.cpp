#include "mesh.h"
#include "matrix.h"
#include "material_list.h"
#include "graphics_state.h"
#include "../engine/opengl_adapter.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace megusta { namespace dts {
OpenGLAdapter* dtsRenderGL=nullptr;
// 004c248f uses SAR, so the high two flag patterns address the preceding
// words at 005234c4 and 005234c8, rather than modes 2 and 3.
int dtsPrimitiveModeStorage[6]={0x7fff,0xffff,8,9,10,1};
int* dtsPrimitiveModes=dtsPrimitiveModeStorage+2;
// The names describe their roles in the saved multiply; render-state setup
// still needs to initialize these values in the same order as the original.
float meshFadeFactor=1.0f,meshReflectionFactor=1.0f,meshFogFactor=1.0f,meshAlphaFactor=1.0f,meshCurrentAlpha=1.0f;
bool meshFadeActive=false;
int dtsSavedDecalTextureMode=8;
float dtsSavedDecalAlpha=0;
std::uint32_t dtsCurrentMaterialFlags=0,dtsCurrentMaterialIndex=0;
int dtsEnvironmentMode=0,dtsDetailMode=0,dtsActiveTextureUnit=0;
int dtsEnvironmentTextureUnit=0,dtsFogTextureUnit=0;
int& dtsSecondaryTextureUnit=dtsFogTextureUnit;
int dtsTextureMode=8,dtsCurrentDetailIndex=0;
float dtsCurrentIntraDetail=1.0f,dtsDetailMapAlpha=1.0f;
bool dtsTexturingPassEnabled=false;
bool dtsTranslucentObjectRange=false,dtsTranslucentObjectsOnly=false;
bool dtsDecalsEnabled=true;
std::uint32_t dtsEnvironmentTextureName=0;
namespace { DTSMaterialTextureHandle fogTextureHandle; }
DTSMaterialTextureHandle* dtsCurrentEnvironmentTexture=&fogTextureHandle;
float dtsEnvironmentAlpha=0,dtsInitialAlpha=1.0f;
float& dtsRenderAlpha=dtsInitialAlpha;
float dtsFogColor[4]={0,0,0,0};
float& dtsFogRenderAlpha=dtsFogColor[3];
bool dtsFogColorUsesAlpha=false;
float dtsMeshColor[4]={1.0f,1.0f,1.0f,1.0f};
bool dtsMultitextureSupported=false,dtsForceMaterialUpdate=false;
bool dtsExtensionsDetected=false,dtsTextureEnvCombineSupported=false;
bool dtsDisplayListsEnabled=true,dtsMaterialAlphaState=false,dtsMaterialForceDepthWrite=false,dtsSavedLightingState=false;
bool& dtsMaterialAlphaToggle=meshFadeActive;
bool& dtsFogPreviouslyEnabled=dtsSavedLightingState;
bool dtsSkinDisplayListsEnabled=true;
int dtsSkinMode=0;
const DTSMatrix4* dtsCurrentNodeTransforms=nullptr;
const DTSMatrix4* dtsCurrentModelTransform=nullptr;

void detectTextureExtensions() {
    dtsMultitextureSupported=false; dtsTextureEnvCombineSupported=false;
    dtsExtensionsDetected=true;
    const char* extensions=dtsRenderGL->getStringOrPlatform(0xa8);
    if (extensions) {
        dtsMultitextureSupported=std::strstr(extensions,"GL_ARB_multitexture")!=nullptr;
        dtsTextureEnvCombineSupported=std::strstr(extensions,"GL_EXT_texture_env_combine")!=nullptr;
    }
}
bool hasMultitexture() {
    if (!dtsExtensionsDetected) detectTextureExtensions();
    return dtsMultitextureSupported;
}
bool hasTextureEnvCombine() {
    if (!dtsExtensionsDetected) detectTextureExtensions();
    return dtsTextureEnvCombineSupported;
}

void setMeshColor(float red,float green,float blue) {
    dtsMeshColor[0]=red; dtsMeshColor[1]=green; dtsMeshColor[2]=blue; dtsMeshColor[3]=1.0f;
}
void initializeMeshColor() {
    dtsMeshColor[0]=dtsMeshColor[1]=dtsMeshColor[2]=dtsMeshColor[3]=1.0f;
}

void TSMesh::initMaterials() {
    dtsMaterialAlphaToggle=false;
    dtsForceMaterialUpdate=false;
    meshFogFactor=meshFadeFactor=meshReflectionFactor=1.0f;
    meshCurrentAlpha=1.0f;
    meshAlphaFactor=dtsInitialAlpha;
    if (hasTextureEnvCombine()) {
        if (dtsEnvironmentMode==1) {
            dtsRenderGL->activeTexture(dtsEnvironmentTextureUnit+0x8a);
            dtsRenderGL->enable(0x4a);
            dtsRenderGL->texGeni(0x5d,0x5c,0x5f); dtsRenderGL->texGeni(0x5e,0x5c,0x5f);
            dtsRenderGL->enable(0x62); dtsRenderGL->enable(0x63);
            dtsRenderGL->texEnvi(0x48,0x49,0x8e);
            dtsRenderGL->texEnvi(0x48,0x8f,0x9f); dtsRenderGL->texEnvi(0x48,0x91,0x10);
            dtsRenderGL->texEnvi(0x48,0x97,0x33); dtsRenderGL->texEnvi(0x48,0x92,0xa2);
            dtsRenderGL->texEnvi(0x48,0x98,0x33); dtsRenderGL->texEnvi(0x48,0x93,0xa2);
            dtsRenderGL->texEnvi(0x48,0x99,0x37); dtsRenderGL->texEnvi(0x48,0x90,0x65);
            dtsRenderGL->texEnvi(0x48,0x94,0xa2); dtsRenderGL->texEnvi(0x48,0x9a,0x37);
            dtsRenderGL->bindTexture(0x4a,dtsEnvironmentTextureName);
            meshReflectionFactor=dtsEnvironmentAlpha;
            dtsRenderGL->activeTexture(dtsActiveTextureUnit+0x8a);
        }
        if (dtsTextureMode==9) {
            dtsRenderGL->activeTexture(dtsFogTextureUnit+0x8a);
            dtsRenderGL->enable(0x4a); dtsRenderGL->texEnvfv(0x48,100,dtsFogColor);
            dtsRenderGL->texEnvi(0x48,0x49,0x8e); dtsRenderGL->texEnvi(0x48,0x8f,0x9f);
            dtsRenderGL->texEnvi(0x48,0x91,0x10); dtsRenderGL->texEnvi(0x48,0x97,0x33);
            dtsRenderGL->texEnvi(0x48,0x92,0xa2); dtsRenderGL->texEnvi(0x48,0x98,0x33);
            dtsRenderGL->texEnvi(0x48,0x99,0x37); dtsRenderGL->texEnvi(0x48,0x90,0x65);
            dtsRenderGL->texEnvi(0x48,0x94,0xa2); dtsRenderGL->texEnvi(0x48,0x9a,0x37);
            dtsRenderGL->texEnvi(0x48,0x93,dtsFogColorUsesAlpha ? 0x10 : 0xa0);
            dtsRenderGL->bindTexture(0x4a,materialTextureEntryName(dtsCurrentEnvironmentTexture->entry));
            dtsRenderGL->activeTexture(dtsActiveTextureUnit+0x8a);
        }
    }
    dtsCurrentMaterialFlags=3;
    dtsCurrentMaterialIndex=drawPrimitiveFlag(TSDrawPrimitiveFlag::DynamicMaterial);
    dtsRenderGL->enable(0x18); dtsRenderGL->frontFace(0x14);
    dtsRenderGL->enableClientState(0x1b); dtsRenderGL->enableClientState(0x1d); dtsRenderGL->enableClientState(0x1e);
    dtsRenderGL->blendFunc(0x37,0x38); dtsRenderGL->texEnvi(0x48,0x49,0x56);
    dtsRenderGL->disable(0x2e); dtsRenderGL->depthMask(true);
    dtsSavedLightingState=dtsRenderGL->isEnabled(0x67);
    meshCurrentAlpha=meshFogFactor*meshFadeFactor*meshReflectionFactor*meshAlphaFactor;
    dtsMeshColor[3]=meshCurrentAlpha;
    const float color[4]={dtsMeshColor[0],dtsMeshColor[1],dtsMeshColor[2],dtsMeshColor[3]};
    dtsRenderGL->color4fv(color); dtsRenderGL->materialfv(0x15,0x74,color);
    dtsRenderGL->disable(0x4a);
}

void TSMesh::setMaterial(std::uint32_t material,TSMaterialList* materials) {
    if (hasDrawPrimitiveFlag(dtsCurrentMaterialIndex|material,TSDrawPrimitiveFlag::DynamicMaterial)) {
        if (hasDrawPrimitiveFlag(material,TSDrawPrimitiveFlag::DynamicMaterial)) {
            dtsRenderGL->disable(0x4a); dtsRenderGL->disable(0x2e); dtsRenderGL->depthMask(true);
            dtsCurrentMaterialIndex=material; dtsCurrentMaterialFlags&=~4u; return;
        }
        dtsRenderGL->enable(0x4a);
    }
    const std::uint32_t index=drawPrimitiveMaterialIndex(material);
    const std::uint32_t sourceFlags=materials->getFlags(static_cast<int>(index));
    dtsForceMaterialUpdate=false;
    std::uint32_t flags=sourceFlags;
    if (dtsMaterialAlphaState || dtsMaterialAlphaToggle) {
        if ((sourceFlags&4u)==0) dtsForceMaterialUpdate=dtsMaterialForceDepthWrite;
        flags|=4u;
    }
    std::uint32_t changed=dtsCurrentMaterialFlags^flags;
    if (dtsEnvironmentMode!=1) changed&=~0x40u;
    const std::uint32_t texture=materials->getTextureName(static_cast<int>(index));
    dtsCurrentMaterialFlags=flags; dtsCurrentMaterialIndex=index;
    dtsRenderGL->bindTexture(0x4a,texture);
    if (changed!=0 || dtsForceMaterialUpdate) {
    if (changed&0x40u) {
        dtsRenderGL->activeTexture(dtsEnvironmentTextureUnit+0x8a);
        if ((sourceFlags&0x40u)==0) dtsRenderGL->enable(0x4a); else dtsRenderGL->disable(0x4a);
        dtsRenderGL->activeTexture(dtsActiveTextureUnit+0x8a);
    }
    if ((flags&4u)==0) {
        if (dtsTextureMode==11) meshFogFactor=1.0f;
        else if (dtsTextureMode==9) {
            dtsRenderGL->activeTexture(dtsFogTextureUnit+0x8a); dtsRenderGL->enable(0x4a);
            dtsRenderGL->activeTexture(dtsActiveTextureUnit+0x8a); meshFogFactor=1.0f;
        }
        dtsRenderGL->disable(0x2e); dtsRenderGL->depthMask(true);
    } else {
        if (dtsTextureMode==11) meshFogFactor=1.0f-dtsFogRenderAlpha;
        else if (dtsTextureMode==9) {
            dtsRenderGL->activeTexture(dtsFogTextureUnit+0x8a); dtsRenderGL->disable(0x4a);
            dtsRenderGL->activeTexture(dtsActiveTextureUnit+0x8a); meshFogFactor=1.0f-dtsFogRenderAlpha;
        }
        dtsRenderGL->enable(0x2e);
        if (!dtsForceMaterialUpdate) dtsRenderGL->depthMask(false);
    }
    if (changed&0x18u) {
        int source,destination;
        if (flags&8u) { source=0x37; destination=0x32; }
        else if (flags&0x10u) { source=0x31; destination=0x34; }
        else { source=0x37; destination=0x38; }
        dtsRenderGL->blendFunc(source,destination);
    }
    if (changed&0x20u) {
        if ((flags&0x20u)==0 && dtsSavedLightingState) dtsRenderGL->enable(0x67);
        else dtsRenderGL->disable(0x67);
    }
    }
    dtsRenderGL->texParameteri(0x4a,0x4b,0x58-((flags&1u)!=0));
    dtsRenderGL->texParameteri(0x4a,0x4c,0x58-((flags&2u)!=0));
    if ((flags&0x40u)==0 && dtsEnvironmentMode!=3)
        meshReflectionFactor=materials->reflectionAmounts[index]*dtsEnvironmentAlpha;
    else meshReflectionFactor=1.0f;
    // Preserve the saved 004c1a40 multiply order (alpha, fade, reflection,
    // fog); this order matters under the original x87 evaluation model.
    const float alpha=meshAlphaFactor*meshFadeFactor*meshReflectionFactor*meshFogFactor;
    const float delta=meshCurrentAlpha-alpha; meshCurrentAlpha=alpha;
    if (std::fabs(delta)>0.001) {
        const float color[4]={1.0f,1.0f,1.0f,alpha};
        dtsRenderGL->color4fv(color); dtsRenderGL->materialfv(0x15,0x74,color);
    }
}

void TSMesh::render(int frame,int materialFrame,TSMaterialList* materials) {
    renderUsingBuffers(frame,materialFrame,materials,nullptr,nullptr);
}

void TSSortedMesh::render(int frame,int materialFrame,TSMaterialList* materials) {
    float view[16]={};
    float inverseView[16]={};
    FUN_004d46e0(view);
    invertAffineMatrix(view,inverseView);
    const float cameraX=inverseView[3];
    const float cameraY=inverseView[7];
    const float cameraZ=inverseView[11];

    const int vertexOffset=firstVertices[static_cast<std::size_t>(frame)];
    if (materialFrame==0) materialFrame=frame;
    const int texcoordOffset=firstTexcoords[static_cast<std::size_t>(materialFrame)];
    dtsRenderGL->vertexPointer(3,3,0,vertexData()+vertexOffset);
    dtsRenderGL->normalPointer(3,0,getNormals(vertexOffset));
    dtsRenderGL->texCoordPointer(2,3,0,texcoords.data()+texcoordOffset*2);
    if (alwaysWriteDepth) dtsRenderGL->depthMask(true);

    int clusterIndex=startCluster[static_cast<std::size_t>(frame)];
    while (clusterIndex>=0) {
        const TSSortedMeshCluster& cluster=clusters[static_cast<std::size_t>(clusterIndex)];
        for (int primitiveIndex=cluster.startPrimitive;
             primitiveIndex<cluster.endPrimitive;++primitiveIndex) {
            const TSDrawPrimitive& primitive=primitives[static_cast<std::size_t>(primitiveIndex)];
            if (((primitive.material^dtsCurrentMaterialIndex)&drawPrimitiveMaterialCacheMask)!=0) {
                TSMesh::setMaterial(primitive.material,materials);
                if (alwaysWriteDepth) dtsRenderGL->depthMask(true);
            }
            const int mode=drawPrimitiveModeIndex(primitive.material);
            dtsRenderGL->drawElements(dtsPrimitiveModes[mode],
                static_cast<std::int16_t>(primitive.count),2,
                indices.data()+static_cast<std::int16_t>(primitive.start));
        }
        if (cluster.frontCluster==cluster.backCluster ||
            cluster.k<cameraX*cluster.normal[0]+cameraY*cluster.normal[1]+cameraZ*cluster.normal[2])
            clusterIndex=cluster.frontCluster;
        else
            clusterIndex=cluster.backCluster;
    }
    if ((dtsCurrentMaterialFlags&4u) && alwaysWriteDepth) dtsRenderGL->depthMask(false);
}

void TSDecalMesh::render(int frame,int materialFrame,TSMaterialList* materials) {
    if (!targetMesh || texgenS.empty() || texgenT.empty()) return;
    std::uint32_t type=meshType;
    if (type&0x08000000u) {
        if (!dtsDisplayListsEnabled) {
            if (type&0x04000000u) {
                dtsRenderGL->deleteLists(displayList,1);
                displayList=0;
                type&=~0x04000000u;
                meshType=static_cast<std::uint32_t>(type);
            }
        } else if (type&0x04000000u) {
            dtsRenderGL->callList(displayList);
            return;
        } else if ((hasDynamicMaterials(materials)&0xffu)==0 &&
                   (static_cast<std::uint32_t>(targetMesh->meshType)&0xa0000000u)==0) {
            displayList=dtsRenderGL->generateLists(1);
        } else {
            displayList=0;
            type&=~0x0c000000u;
            meshType=static_cast<std::uint32_t>(type);
        }
        if (displayList) {
            dtsRenderGL->newList(displayList,0xa3);
            type|=0x04000000u;
            meshType=static_cast<std::uint32_t>(type);
        }
    }

    const int vertexOffset=targetMesh->verticesPerFrame*frame;
    dtsRenderGL->vertexPointer(3,3,0,targetMesh->vertexData()+vertexOffset);
    dtsRenderGL->normalPointer(3,0,targetMesh->getNormals(vertexOffset));
    dtsRenderGL->texGenfv(0x5d,0x60,texgenS[static_cast<std::size_t>(materialFrame)].values);
    dtsRenderGL->texGenfv(0x5e,0x60,texgenT[static_cast<std::size_t>(materialFrame)].values);
    if (((static_cast<std::uint32_t>(materialIndex)^dtsCurrentMaterialIndex)&drawPrimitiveMaterialCacheMask)!=0)
        TSMesh::setMaterial(static_cast<std::uint32_t>(materialIndex),materials);
    int first=startPrimitive[static_cast<std::size_t>(materialFrame)];
    const int end=materialFrame+1<static_cast<int>(startPrimitive.size())
        ? startPrimitive[static_cast<std::size_t>(materialFrame+1)]
        : static_cast<int>(primitives.size());
    for (;first<end;++first) {
        const TSDrawPrimitive& primitive=primitives[static_cast<std::size_t>(first)];
        const int mode=drawPrimitiveModeIndex(primitive.material);
        dtsRenderGL->drawElements(dtsPrimitiveModes[mode],
            static_cast<std::int16_t>(primitive.count),2,
            indices.data()+static_cast<std::int16_t>(primitive.start));
    }
    if (static_cast<std::uint32_t>(meshType)&0x04000000u) {
        dtsRenderGL->endList();
        dtsRenderGL->callList(displayList);
    }
}
void TSMesh::renderUsingBuffers(int frame,int materialFrame,TSMaterialList* materials,
                                const TSMeshPoint* vertexBuffer,const TSMeshPoint* normalBuffer) {
    std::uint32_t type=static_cast<std::uint32_t>(meshType);
    if (type&0x08000000u) {
        if (!dtsDisplayListsEnabled) {
            if (type&0x04000000u) {
                dtsRenderGL->deleteLists(displayList,1); displayList=0;
                meshType=static_cast<int>(type&~0x04000000u); type=static_cast<std::uint32_t>(meshType);
            }
        } else if (type&0x04000000u) {
            dtsRenderGL->callList(displayList); return;
        } else if ((hasDynamicMaterials(materials)&0xffu)!=0) {
            displayList=0; meshType=static_cast<int>(type&~0x0c000000u); type=static_cast<std::uint32_t>(meshType);
        } else {
            displayList=dtsRenderGL->generateLists(1);
            if (displayList) {
                dtsRenderGL->newList(displayList,0xa3);
                meshType=static_cast<int>(type|0x04000000u); type|=0x04000000u;
            }
        }
    }
    const int vertexOffset=verticesPerFrame*frame;
    if (static_cast<std::uint32_t>(meshType)&0x80000000u)
        forceFaceCamera();
    dtsRenderGL->vertexPointer(3,3,0,(vertexBuffer ? vertexBuffer : vertexData())+vertexOffset);
    dtsRenderGL->normalPointer(3,0,normalBuffer ? normalBuffer+vertexOffset : getNormals(vertexOffset));
    dtsRenderGL->texCoordPointer(2,3,0,texcoords.data()+verticesPerFrame*materialFrame*2);
    for (std::size_t primitive=0;primitive<primitives.size();++primitive) {
        const std::uint32_t material=primitives[primitive].material;
        if (((material^dtsCurrentMaterialIndex)&drawPrimitiveMaterialCacheMask)!=0 || dtsForceMaterialUpdate)
            setMaterial(material,materials);
        const int mode=drawPrimitiveModeIndex(material);
        dtsRenderGL->drawElements(dtsPrimitiveModes[mode],static_cast<std::int16_t>(primitives[primitive].count),2,
            indices.data()+static_cast<std::int16_t>(primitives[primitive].start));
    }
    if (static_cast<std::uint32_t>(meshType)&0x04000000u) {
        dtsRenderGL->endList(); dtsRenderGL->callList(displayList);
    }
}

void TSSkinMesh::render(int frame,int materialFrame,TSMaterialList* materials) {
    if (!dtsSkinDisplayListsEnabled) {
        if (static_cast<std::uint32_t>(meshType)&0x04000000u) {
            dtsRenderGL->deleteLists(displayList,1); displayList=0;
            meshType=static_cast<int>(static_cast<std::uint32_t>(meshType)&~0x04000000u);
        }
        meshType=static_cast<int>(static_cast<std::uint32_t>(meshType)&~0x08000000u);
    }
    if (dtsSkinMode==1) {
        updateSkin();
    }
    // The redirected base streams survive a later render with skin updates
    // disabled, just as the saved base pointers do.
    TSMesh::render(frame,materialFrame,materials);
}

void TSMesh::renderEnvironmentMap(int frame,int materialFrame,TSMaterialList* materials) {
    (void)materialFrame;
    const int vertexOffset=verticesPerFrame*frame;
    dtsRenderGL->vertexPointer(3,3,0,vertexData()+vertexOffset);
    dtsRenderGL->normalPointer(3,0,normalData()+vertexOffset);
    std::uint32_t type=static_cast<std::uint32_t>(meshType);
    if (type&0x08000000u) {
        if (!dtsDisplayListsEnabled) {
            if (type&0x02000000u) {
                dtsRenderGL->deleteLists(environmentDisplayList,1); environmentDisplayList=0;
                meshType=static_cast<int>(type&~0x02000000u);
            }
        } else if (type&0x02000000u) { dtsRenderGL->callList(environmentDisplayList); return; }
        else {
            environmentDisplayList=dtsRenderGL->generateLists(1);
            if (environmentDisplayList) { dtsRenderGL->newList(environmentDisplayList,0xa3); meshType=static_cast<int>(type|0x02000000u); }
        }
    }
    std::uint32_t previous=0xffffffffu;
    for (const auto& primitive:primitives) {
        const std::uint32_t material=primitive.material;
        if (((material^previous)&drawPrimitiveMaterialCacheMask)!=0) {
            previous=material&drawPrimitiveMaterialCacheMask;
            if (!hasDrawPrimitiveFlag(material,TSDrawPrimitiveFlag::DynamicMaterial) &&
                !hasMaterialFlag(materials->getFlags(static_cast<int>(previous)),
                                 TSMaterialFlag::NeverEnvironmentMap)) {
                meshReflectionFactor=materials->reflectionAmounts[previous]*dtsEnvironmentAlpha;
                // Saved 004c1e40 evaluates fog, alpha, fade, then reflection.
                const float alpha=meshFogFactor*meshAlphaFactor*meshFadeFactor*meshReflectionFactor;
                const float delta=meshCurrentAlpha-alpha; meshCurrentAlpha=alpha;
                if (std::fabs(delta)>0.001) {
                    const float color[4]={1.0f,1.0f,1.0f,alpha};
                    dtsRenderGL->color4fv(color); dtsRenderGL->materialfv(0x15,0x74,color);
                }
            } else continue;
        }
        const int mode=drawPrimitiveModeIndex(material);
        dtsRenderGL->drawElements(dtsPrimitiveModes[mode],static_cast<std::int16_t>(primitive.count),2,
            indices.data()+static_cast<std::int16_t>(primitive.start));
    }
    if (static_cast<std::uint32_t>(meshType)&0x02000000u) {
        dtsRenderGL->endList(); dtsRenderGL->callList(environmentDisplayList);
    }
}

void TSMesh::initEnvironmentMapMaterials() {
    dtsRenderGL->bindTexture(0x4a,dtsEnvironmentTextureName);
    dtsRenderGL->texGeni(0x5d,0x5c,0x5f);
    dtsRenderGL->texGeni(0x5e,0x5c,0x5f);
    dtsRenderGL->enable(0x62); dtsRenderGL->enable(0x63);
    dtsRenderGL->enable(0x4a); dtsRenderGL->enable(0x18);
    dtsRenderGL->frontFace(0x14);
    dtsRenderGL->enableClientState(0x1b); dtsRenderGL->enableClientState(0x1d);
    dtsRenderGL->blendFunc(0x37,0x38);
    dtsRenderGL->texEnvi(0x48,0x49,0x56);
    dtsRenderGL->enable(0x2e);
    meshCurrentAlpha=dtsInitialAlpha*dtsEnvironmentAlpha;
    meshFogFactor=1; meshFadeFactor=1;
    meshReflectionFactor=dtsEnvironmentAlpha; meshAlphaFactor=dtsInitialAlpha;
    const float color[4]={1,1,1,meshCurrentAlpha};
    dtsRenderGL->color4fv(color); dtsRenderGL->materialfv(0x15,0x74,color);
}

void TSMesh::resetMaterials() {
    if (dtsEnvironmentMode==1) {
        dtsRenderGL->activeTexture(dtsEnvironmentTextureUnit+0x8a);
        dtsRenderGL->texEnvi(0x48,0x49,0x65);
        dtsRenderGL->disable(0x4a); dtsRenderGL->disable(0x62); dtsRenderGL->disable(0x63);
    }
    if (dtsTextureMode==9) {
        dtsRenderGL->activeTexture(dtsFogTextureUnit+0x8a);
        dtsRenderGL->texEnvi(0x48,0x49,0x65);
        dtsRenderGL->disable(0x4a);
    }
    if (hasMultitexture()) dtsRenderGL->activeTexture(dtsActiveTextureUnit+0x8a);
    dtsRenderGL->disableClientState(0x1b);
    dtsRenderGL->disableClientState(0x1d);
    dtsRenderGL->disableClientState(0x1e);
    dtsRenderGL->disable(0x4a); dtsRenderGL->disable(0x2e);
    dtsRenderGL->depthMask(true); dtsRenderGL->enable(0x18);
    if (dtsFogPreviouslyEnabled && !dtsRenderGL->isEnabled(0x67)) dtsRenderGL->enable(0x67);
    if (dtsActiveTextureUnit!=0) {
        dtsRenderGL->activeTexture(0x8a);
        dtsRenderGL->clientActiveTexture(0x8a);
    }
    dtsForceMaterialUpdate=false;
}

void TSDecalMesh::initDecalMaterials() {
    dtsCurrentMaterialFlags=3;
    dtsCurrentMaterialIndex=drawPrimitiveFlag(TSDrawPrimitiveFlag::DynamicMaterial);
    dtsEnvironmentMode=0; dtsDetailMode=4; dtsActiveTextureUnit=0;
    dtsRenderGL->enable(0x18);
    dtsRenderGL->frontFace(0x14);
    dtsRenderGL->enableClientState(0x1b);
    dtsRenderGL->enableClientState(0x1d);
    dtsRenderGL->texGeni(0x5d,0x5c,0x61);
    dtsRenderGL->texGeni(0x5e,0x5c,0x61);
    dtsRenderGL->enable(0x62);
    dtsRenderGL->enable(0x63);
    dtsRenderGL->enable(0x19);
    dtsRenderGL->polygonOffset(-2.0f,-2.0f);
    dtsRenderGL->texEnvi(0x48,0x49,0x56);
    dtsRenderGL->blendFunc(0x37,0x38);
    dtsRenderGL->disable(0x2e);
    dtsRenderGL->depthMask(true);
    dtsRenderGL->disable(0x4a);
    dtsSavedDecalTextureMode=dtsTextureMode;
    if (dtsTextureMode!=8) dtsSavedDecalAlpha=dtsInitialAlpha;
    dtsTextureMode=11;
}

void TSMesh::resetEnvironmentMapMaterials() {
    dtsRenderGL->disable(0x4a);
    dtsRenderGL->disable(0x2e);
    dtsRenderGL->depthMask(true);
    dtsRenderGL->enable(0x18);
    dtsRenderGL->disableClientState(0x1b);
    dtsRenderGL->disableClientState(0x1d);
    dtsRenderGL->disableClientState(0x1e);
    dtsRenderGL->disable(0x62);
    dtsRenderGL->disable(0x63);
    dtsRenderGL->disable(0x4a);
}
void TSDecalMesh::resetDecalMaterials() {
    dtsRenderGL->disableClientState(0x1b);
    dtsRenderGL->disableClientState(0x1d);
    dtsRenderGL->disable(0x62);
    dtsRenderGL->disable(0x63);
    dtsRenderGL->disable(0x19);
    dtsRenderGL->disable(0x4a);
    dtsRenderGL->disable(0x2e);
    dtsRenderGL->depthMask(true);
    if (dtsSavedDecalTextureMode!=8) dtsInitialAlpha=dtsSavedDecalAlpha;
    dtsTextureMode=dtsSavedDecalTextureMode;
}

void TSMesh::setFade(float value) {
    meshFadeFactor=value;
    float alpha=meshReflectionFactor*meshFogFactor*meshAlphaFactor*value;
    float change=meshCurrentAlpha-alpha;
    meshCurrentAlpha=alpha;
    if (std::fabs(change)>0.001) {
        const float color[4]={1.0f,1.0f,1.0f,alpha};
        dtsRenderGL->color4fv(color);
        dtsRenderGL->materialfv(0x15,0x74,color);
    }
    meshFadeActive=true;
}
void TSMesh::clearFade() {
    setFade(1.0f);
    meshFadeActive=false;
}

void TSMesh::renderFog(int frame,TSMaterialList* materials) {
    dtsRenderGL->vertexPointer(3,3,0,vertexData()+verticesPerFrame*frame);
    for (const auto& primitive:primitives) {
        const auto flags=materials->getFlags(static_cast<int>(drawPrimitiveMaterialIndex(primitive.material)));
        if ((flags&0xcu)==0) {
            const int mode=drawPrimitiveModeIndex(primitive.material);
            dtsRenderGL->drawElements(dtsPrimitiveModes[mode],static_cast<std::int16_t>(primitive.count),2,
                                      indices.data()+static_cast<std::int16_t>(primitive.start));
        }
    }
}

namespace {
struct ShadowRasterPoint { std::int32_t x,y; };
struct ShadowRasterEdge {
    std::int32_t x,firstY,lastY,step,errorStep,error,denominator,direction;
    bool horizontal;
};
ShadowRasterEdge makeShadowRasterEdge(ShadowRasterPoint first,ShadowRasterPoint second) {
    if (second.y<first.y) std::swap(first,second);
    const std::int32_t dy=second.y-first.y;
    const std::int32_t dx=second.x-first.x;
    if (dy==0) return {first.x,first.y,second.y,0,0,0,0,0,true};
    const std::int32_t step=dx/dy;
    const std::int32_t remainder=dx-step*dy;
    const std::int32_t errorStep=remainder<0?-remainder:remainder;
    // In the saved 004d3970/004d3ee0 loops, the edge error accumulator is
    // initialized to zero and receives the absolute division remainder once
    // per scanline. The remainder's sign is carried separately by direction.
    return {first.x,first.y,second.y,step,errorStep,0,
            dy,dx>=1?1:-1,false};
}
void advanceShadowRasterEdge(ShadowRasterEdge& edge) {
    edge.x+=edge.step;
    edge.error+=edge.errorStep;
    if (edge.error>=edge.denominator) {
        edge.x+=edge.direction;
        edge.error-=edge.denominator;
    }
}
void fillShadowSpan(std::int32_t y,std::int32_t firstX,std::int32_t lastX,
                    int dimension,std::uint32_t* mask) {
    if (lastX<firstX) std::swap(firstX,lastX);
    const std::int32_t pitch=dimension>>5;
    const std::uint32_t firstWord=static_cast<std::uint32_t>(firstX)>>5;
    const std::uint32_t lastWord=static_cast<std::uint32_t>(lastX)>>5;
    const std::uint32_t firstMask=0xffffffffu<<(static_cast<std::uint32_t>(firstX)&31u);
    const std::uint32_t lastMask=0xffffffffu>>(31u-(static_cast<std::uint32_t>(lastX)&31u));
    std::uint32_t* row=mask+y*pitch;
    if (firstWord==lastWord) {
        row[firstWord]|=firstMask&lastMask;
        return;
    }
    row[firstWord]|=firstMask;
    for (std::uint32_t word=firstWord+1;word<lastWord;++word) row[word]=0xffffffffu;
    row[lastWord]|=lastMask;
}
void rasterizeShadowTriangle(std::uint16_t first,std::uint16_t second,std::uint16_t third,
                             const std::int32_t* coordinates,int dimension,std::uint32_t* mask) {
    const auto point=[&](std::uint16_t index) {
        return ShadowRasterPoint{coordinates[index*2],coordinates[index*2+1]};
    };
    const ShadowRasterPoint vertices[3]={point(first),point(second),point(third)};
    ShadowRasterEdge edges[3]={makeShadowRasterEdge(vertices[0],vertices[1]),
                               makeShadowRasterEdge(vertices[1],vertices[2]),
                               makeShadowRasterEdge(vertices[2],vertices[0])};
    const std::int32_t firstY=std::min(vertices[0].y,std::min(vertices[1].y,vertices[2].y));
    const std::int32_t lastY=std::max(vertices[0].y,std::max(vertices[1].y,vertices[2].y));
    for (std::int32_t y=firstY;y<=lastY;++y) {
        std::int32_t intersections[6];
        int count=0;
        for (auto& edge:edges) {
            if (y<edge.firstY || y>edge.lastY) continue;
            if (edge.horizontal) {
                for (int side=0;side<3;++side) {
                    const auto& a=vertices[side];
                    const auto& b=vertices[(side+1)%3];
                    if (a.y==y && b.y==y) { intersections[count++]=a.x; intersections[count++]=b.x; }
                }
                // A horizontal edge is the complete span at this scanline.
                // The other two edges meet its endpoints, so processing them
                // again would only duplicate boundary coordinates.
                break;
            }
            if (y>edge.firstY) advanceShadowRasterEdge(edge);
            intersections[count++]=edge.x;
        }
        if (count<2) continue;
        auto bounds=std::minmax_element(intersections,intersections+count);
        fillShadowSpan(y,*bounds.first,*bounds.second,dimension,mask);
    }
}
int shadowTriangleOrientation(const std::int32_t* coordinates,std::uint16_t a,
                              std::uint16_t b,std::uint16_t c) {
    const std::uint32_t ax=static_cast<std::uint32_t>(coordinates[a*2]);
    const std::uint32_t ay=static_cast<std::uint32_t>(coordinates[a*2+1]);
    const std::uint32_t bx=static_cast<std::uint32_t>(coordinates[b*2]);
    const std::uint32_t by=static_cast<std::uint32_t>(coordinates[b*2+1]);
    const std::uint32_t cx=static_cast<std::uint32_t>(coordinates[c*2]);
    const std::uint32_t cy=static_cast<std::uint32_t>(coordinates[c*2+1]);
    // Both saved rasterizers subtract and multiply 32-bit coordinates before
    // signed comparisons. Express the arithmetic as unsigned modulo-2^32,
    // then reinterpret the result, so overflow follows the x86 instructions
    // instead of relying on C++ signed-overflow behavior.
    const std::uint32_t abx=ax-bx, acy=cy-by, cbx=cx-bx, aby=ay-by;
    const std::uint32_t bits=abx*acy-cbx*aby;
    std::int32_t orientation;
    std::memcpy(&orientation,&bits,sizeof(orientation));
    return orientation;
}
const TSDrawPrimitive& shadowPrimitive(const TSDrawPrimitive* primitives,int index,int stride) {
    return *reinterpret_cast<const TSDrawPrimitive*>(
        reinterpret_cast<const std::uint8_t*>(primitives)+index*stride);
}
}

void rasterizeTriangleStripShadows(const TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
                                   const std::uint16_t* indices,const std::int32_t* coordinates,
                                   int dimension,std::uint32_t* mask) {
    for (int primitiveIndex=0;primitiveIndex<primitiveCount;++primitiveIndex) {
        const auto& primitive=shadowPrimitive(primitives,primitiveIndex,primitiveStride);
        const int count=static_cast<std::int16_t>(primitive.count);
        const int start=static_cast<std::int16_t>(primitive.start);
        for (int vertex=2;vertex<count;++vertex) {
            auto a=indices[start+vertex-2],b=indices[start+vertex-1],c=indices[start+vertex];
            if (vertex&1) std::swap(a,b);
            // The original strip walker tests that each of the three vertex
            // records is distinct before computing orientation or rasterizing.
            if (a==b || b==c || c==a) continue;
            // 004d3970 keeps the strip triangle when its saved integer
            // orientation expression is non-positive. The list routine uses
            // the same orientation helper and returns on a positive result.
            if (shadowTriangleOrientation(coordinates,a,b,c)<=0)
                rasterizeShadowTriangle(a,b,c,coordinates,dimension,mask);
        }
    }
}

void rasterizeTriangleListShadows(const TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
                                  const std::uint16_t* indices,const std::int32_t* coordinates,
                                  int dimension,std::uint32_t* mask) {
    for (int primitiveIndex=0;primitiveIndex<primitiveCount;++primitiveIndex) {
        const auto& primitive=shadowPrimitive(primitives,primitiveIndex,primitiveStride);
        const int count=static_cast<std::int16_t>(primitive.count);
        const int start=static_cast<std::int16_t>(primitive.start);
        for (int vertex=0;vertex<count;vertex+=3) {
            const auto a=indices[start+vertex],b=indices[start+vertex+1],c=indices[start+vertex+2];
            if (shadowTriangleOrientation(coordinates,a,b,c)>0) return;
            rasterizeShadowTriangle(a,b,c,coordinates,dimension,mask);
        }
    }
}

void TSMesh::renderShadow(int frame,const float* transform,int dimension,std::uint32_t* mask,
                          TSMaterialList* materials) {
    if (verticesPerFrame==0 || vertexCount()==0 || primitives.empty()) return;
    const std::uint32_t firstMaterial=primitives[0].material;
    if (!hasDrawPrimitiveFlag(firstMaterial,TSDrawPrimitiveFlag::DynamicMaterial) &&
        hasMaterialFlag(materials->getFlags(drawPrimitiveMaterialIndex(firstMaterial)),TSMaterialFlag::Translucent))
        return;
    shadowCoordinates.resize(static_cast<std::size_t>(verticesPerFrame)*2);
    const int vertexOffset=verticesPerFrame*frame;
    shadowTransform(transform,vertexData()+vertexOffset,
                    vertexData()+vertexOffset+verticesPerFrame,dimension-1);
    const auto primitiveMode=drawPrimitiveType(firstMaterial);
    if (primitiveMode==TSDrawPrimitiveType::TriangleStrip)
        rasterizeTriangleStripShadows(primitives.data(),static_cast<int>(primitives.size()),
            static_cast<int>(sizeof(TSDrawPrimitive)),indices.data(),shadowCoordinates.data(),dimension,mask);
    else if (primitiveMode==TSDrawPrimitiveType::TriangleList)
        rasterizeTriangleListShadows(primitives.data(),static_cast<int>(primitives.size()),
            static_cast<int>(sizeof(TSDrawPrimitive)),indices.data(),shadowCoordinates.data(),dimension,mask);
}
} }

extern "C" void MEGUSTA_DTS_CDECL DTS_registeredStartup_004c1550(void) {
    megusta::dts::initializeMeshColor();
}
extern "C" void MEGUSTA_DTS_CDECL DTS_staticInitializer_004c1560(void) {
    megusta::dts::initializeMeshColor();
}
extern "C" bool FUN_004d48d0(void) {
    return megusta::dts::hasMultitexture();
}
extern "C" bool FUN_004d48f0(void) {
    return megusta::dts::hasTextureEnvCombine();
}
