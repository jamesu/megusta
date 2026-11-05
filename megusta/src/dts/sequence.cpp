#include "sequence.h"
#include "../engine/resource_file.h"
namespace megusta { namespace dts {
std::uint32_t currentShapeReadVersion=0;
std::vector<std::int32_t> legacyFirstKeyframeScratch;
void TSShapeSequence::read(ResourceFile* file,int readNameIndex) {
    read(*file,currentShapeReadVersion,static_cast<unsigned char>(readNameIndex)!=0,legacyFirstKeyframeScratch);
}
void TSShapeSequence::read(ResourceFile& file,std::uint32_t version,bool readNameIndex,std::vector<std::int32_t>& legacyFirstKeyframes) {
    auto word=[&](void* p) { file.read(p,4,1); };
    if (readNameIndex) word(&nameIndex);
    if (version<22) flags=0; else word(&flags);
    if (version<17) {
        std::int32_t first=0,last=0; word(&first); word(&last);
        numKeyframes=last-first; legacyFirstKeyframes.push_back(first);
    } else word(&numKeyframes);
    word(&duration);
    if (version<22) {
        for (unsigned bit=8;bit<=32;bit<<=1) { std::uint8_t value=0; file.read(&value,1,1); if (value) flags|=bit; }
    }
    word(&priority); word(&firstGroundFrame); word(&numGroundFrames);
    if (version<22) {
        if (version>16) { word(&baseRotation); baseTranslation=baseRotation; word(&baseObjectState); word(&baseDecalState); }
    } else { word(&baseRotation); word(&baseTranslation); word(&baseScale); word(&baseObjectState); word(&baseDecalState); }
    if (version<9) { firstTrigger=0; numTriggers=0; } else { word(&firstTrigger); word(&numTriggers); }
    if (version<8) toolBegin=0; else word(&toolBegin);
    rotationMatters.read(&file);
    if (version<22) translationMatters.copy(rotationMatters); else { translationMatters.read(&file); scaleMatters.read(&file); }
    TSIntegerSet discarded;
    if (version<17) discarded.read(&file);
    if (version>10) decalMatters.read(&file);
    if (version>5) iflMatters.read(&file);
    visMatters.read(&file); frameMatters.read(&file); matFrameMatters.read(&file);
    if (version<17) discarded.read(&file);
    dirtyFlags=0;
    if (rotationMatters.testAll(TSIntegerSet::capacity) || translationMatters.testAll(TSIntegerSet::capacity) || scaleMatters.testAll(TSIntegerSet::capacity)) dirtyFlags|=1;
    if (visMatters.testAll(TSIntegerSet::capacity)) dirtyFlags|=2;
    if (frameMatters.testAll(TSIntegerSet::capacity)) dirtyFlags|=4;
    if (matFrameMatters.testAll(TSIntegerSet::capacity)) dirtyFlags|=8;
    if (decalMatters.testAll(TSIntegerSet::capacity)) dirtyFlags|=16;
    if (iflMatters.testAll(TSIntegerSet::capacity)) dirtyFlags|=32;
}
} }

