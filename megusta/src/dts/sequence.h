// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "integer_set.h"
#include <cstdint>
#include <vector>
namespace megusta { class ResourceFile; namespace dts {
extern std::uint32_t currentShapeReadVersion;
extern std::vector<std::int32_t> legacyFirstKeyframeScratch;
struct TSShapeSequence {
    std::int32_t nameIndex=0,numKeyframes=0;
    float duration=0;
    std::int32_t baseRotation=0,baseTranslation=0,baseScale=0,baseObjectState=0,baseDecalState=0;
    std::int32_t firstGroundFrame=0,numGroundFrames=0,firstTrigger=0,numTriggers=0;
    float toolBegin=0;
    TSIntegerSet rotationMatters,translationMatters,scaleMatters,visMatters,frameMatters,matFrameMatters,decalMatters,iflMatters;
    std::int32_t priority=0;
    std::uint32_t flags=0,dirtyFlags=0;
    void read(ResourceFile* file,int readNameIndex);
    // Explicit-version adapter for the normalized loader.
    void read(ResourceFile& file,std::uint32_t version,bool readNameIndex,std::vector<std::int32_t>& legacyFirstKeyframes);
};
static_assert(sizeof(TSShapeSequence)==256,"DTS sequence record width");
} }

