#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "mesh.h"
#include "integer_set.h"
#include "shape.h"
#include "vector_storage.h"
namespace megusta { namespace dts {
// Recovered snapshot globals: upscale ceiling 005234f0 and the raw OpenGL
// correction matrix at 00523858. Initial values are pinned in dts_init_globals.
extern int snapshotMaximumUpscale;
extern float snapshotProjectionCorrection[16];
struct DTSProjectionState {
    float left=0,right=0,bottom=0,top=0,nearPlane=0,farPlane=0;
    bool orthographic=false;
    int viewport[4]={};
    float viewportWidthScale=0,pixelsPerUnit=0;
};
extern DTSProjectionState dtsProjectionState;
class TSShape;
class TSMaterialList;
class TSThread;
struct DTSObject;
struct DTSDecal;
struct DTSIflMaterial;
struct DTSAssembledMesh;
struct TSMeshObjectInstance {
    int nodeIndex=0;
    DTSAssembledMesh* meshList=nullptr;
    const DTSObject* object=nullptr;
    float visible=0;
    int frame=0,materialFrame=0;
    void render(int meshIndex,TSMaterialList* materials);
    void renderEnvironmentMap(int meshIndex,TSMaterialList* materials);
    void renderFog(int meshIndex,TSMaterialList* materials);
    void renderShadow(int meshIndex,const float* worldTransform,int dimension,std::uint32_t* mask,
                      TSMaterialList* materials);
};
struct TSDecalObjectInstance {
    int nodeIndex=0;
    DTSAssembledMesh* meshList=nullptr;
    TSMeshObjectInstance* target=nullptr;
    const DTSDecal* decal=nullptr;
    int frame=0;
    void render(int meshIndex,TSMaterialList* materials);
};
struct TSIflMaterialInstance { const DTSIflMaterial* material=nullptr; int frame=-1; };
// Recovered animate-node callback ABI: callback(0, callbackData, nodeIndex)
// returns the 4x4 local transform to install for that node.
using NodeTransformCallback = const float* (MEGUSTA_DTS_CDECL *)(int,void*,int);
// Normalized runtime representation. Rendering, texture handles, IFLs,
// animation state, and instance-owned material lifetime use C++ containers
// rather than the executable's original pointer/count layout.
class TSShapeInstance {
public:
    TSShape* shape;
    TSMaterialList* materialList=nullptr;
    // 004c6da0 initializes mOwnMaterialList; setMaterialList releases an
    // instance-created list through its scalar deleting destructor.
    bool ownsMaterialList=false;
    // Saved TSShapeInstance::buildInstanceData clears mData at 004c6da0;
    // its consumer/semantic role has not yet been recovered.
    void* instanceData=nullptr;
    int currentDetailLevel=0;
    float currentIntraDetailLevel=1.0f;
    bool alphaIsReflectanceMap=true;
    bool mirrorYRender=false;
    int maximumEnvironmentMapDetail=1,maximumDetailMapDetail=-1;
    bool environmentMapOn=false,environmentMapEntryPresent=false;
    void* environmentTextureEntry=nullptr;
    bool allowTwoPassEnvironmentMap=true,allowTwoPassDetailMap=false,alphaAlways=false;
    float environmentMapAlpha=0.0f,alphaAlwaysValue=1.0f;
    std::uint32_t environmentTextureName=0;
    std::vector<TSMeshObjectInstance> meshObjects;
    std::vector<TSDecalObjectInstance> decalObjects;
    std::vector<TSIflMaterialInstance> iflMaterialInstances;
    std::vector<DTSMatrix4> nodeTransforms;
    std::vector<std::array<float,4>> nodeRotationValues;
    // The executable captures transition reference rotations in Quat16 form,
    // then expands them again while blending. Keep that quantization in the
    // normalized instance instead of retaining extra float precision.
    std::vector<DTSQuat16> nodeReferenceRotations;
    std::vector<DTSPoint3> nodeLocalTranslations,nodeReferenceTranslations;
    std::vector<TSThread*> nodeRotationThreads,nodeTranslationThreads;
    std::vector<float> uniformNodeScales;
    std::vector<DTSPoint3> alignedNodeScales;
    std::vector<DTSArbitraryScale> arbitraryNodeScales;
    std::vector<float> referenceUniformNodeScales;
    std::vector<DTSPoint3> referenceAlignedNodeScales;
    std::vector<DTSPoint3> referenceArbitraryNodeScaleFactors;
    std::vector<DTSQuat16> referenceArbitraryNodeScaleRotations;
    std::vector<TSThread*> nodeScaleThreads;
    std::vector<std::uint32_t> dirtyFlags;
    std::vector<TSThread*> threads;
    std::vector<TSThread*> transitionThreads;
    TSThread* groundThread=nullptr;
    NodeTransformCallback nodeTransformCallback=nullptr;
    void* nodeTransformCallbackData=nullptr;
    TSIntegerSet transitionRotationNodes,transitionTranslationNodes,transitionScaleNodes;
    TSIntegerSet maskRotationNodes,maskPosXNodes,maskPosYNodes,maskPosZNodes;
    TSIntegerSet disableBlendNodes,handsOffNodes,callbackNodes;
    std::uint32_t triggerStates=0;
    bool scaleCurrentlyAnimated=false;
    bool instanceLinksReady=false;
    TSShapeInstance(TSShape* source,int loadMaterials);
    ~TSShapeInstance();
    TSShapeInstance(const TSShapeInstance&)=delete;
    TSShapeInstance& operator=(const TSShapeInstance&)=delete;
    TSThread* addThread();
    void destroyThread(TSThread* thread);
    void setDirty(std::uint32_t flags);
    void clearDirty(std::uint32_t flags);
    void setTriggerStateBit(std::uint32_t mask,char enabled);
    bool buildInstanceData(TSShape* source,int loadMaterials);
    // Initializes the normalized default local transforms.
    bool setDefaultNodeTransforms();
    // Evaluates ordinary tracks, transition state, callback transforms and blends.
    bool animateNodeTransforms(int subShape=-1);
    void handleDefaultScale(int firstNode,int endNode,TSIntegerSet& handled);
    bool handleAnimatedScale(TSThread* thread,int firstNode,int endNode,TSIntegerSet& handled);
    void handleNodeScale(int firstNode,int endNode);
    void handleTransitionNodes(int firstNode,int endNode);
    void handleBlendSequence(TSThread* thread,int firstNode,int endNode);
    void handleMaskedPositionNode(TSThread* thread,int node,int trackRank);
    void updateTransitions();
    std::uint8_t getNodeAnimationState(int node) const;
    void setNodeAnimationState(int node,std::uint32_t state);
    void setNodeTransformCallback(NodeTransformCallback callback,void* callbackData);
    void animateNodeSubtrees(bool markDirty);
    void animateSubtrees(bool markDirty);
    bool animateObjectStates(int subShape,std::uint32_t mask);
    void animateVisibility(int subShape);
    void animateFrame(int subShape);
    void animateMatFrame(int subShape);
    bool animateDecalStates(int subShape);
    void animateDecals(std::uint32_t subShape);
    void animateIfls();
    bool updateIflFrames();
    void setSequence(TSThread* thread,int sequence,float position);
    void transitionToSequence(TSThread* thread,int sequence,float position,float duration,bool targetScale);
    void clearTransition(TSThread* thread);
    void advanceTime(float delta);
    void checkScaleCurrentlyAnimated();
    void animate(int detail);
    void animate();
    void setMaterialList(TSMaterialList* materials);
    void setupDetailMapMeshes();
    void setRenderStatics(int detail,float intraDetail,const float* postScale);
    void setupTexturing(int detail,float intraDetail);
    void renderEnvironmentMap();
    void renderFog();
    void renderShadow(int detail,const float* worldTransform,int dimension,std::uint32_t* mask);
    void renderDetail(int detail,float intraDetail,const float* postScale);
    void renderCurrent(const float* postScale);
    int getCurrentDetail() const;
    void setCurrentDetail(int detail,float intraDetail);
    EngineBitmapView* snapshot(int width,int height,char generateMipmaps,
                               float* transform,char allowUpscale);
};
EngineBitmapView* snapshotShapeStatic(TSShape* shape,int width,int height,
        char generateMipmaps,float* transform,int detail,float intraDetail,char allowUpscale);
} }


