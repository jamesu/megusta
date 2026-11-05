#include "../src/dts/shape.h"
#include "../src/engine/resource_file.h"
#include <cstdio>
#include <cstdlib>

using namespace megusta;
using namespace megusta::dts;

static ResourceFile* openFile(const char* path) {
    std::FILE* file=std::fopen(path,"rb");
    return file ? new ResourceFile(file) : nullptr;
}

int main(int argc,char** argv) {
    if (argc!=3) return 2;
    auto shapeFile=openFile(argv[1]);
    auto sequenceFile=openFile(argv[2]);
    if (!shapeFile || !sequenceFile) return 3;
    TSShape shape;
    std::puts("reading header"); std::fflush(stdout);
    if (shape.readHeader(*shapeFile)!=TSShape::HeaderResult::legacyUnified) return 4;
    std::puts("reading legacy prefix"); std::fflush(stdout);
    if (!shape.readLegacyPrefix(*shapeFile)) return 4;
    std::puts("reading legacy tail"); std::fflush(stdout);
    if (!shape.readLegacyTail(*shapeFile,"")) return 4;
    std::puts("assembling legacy meshes"); std::fflush(stdout);
    if (!shape.assembleLegacyMeshes(false)) return 4;
    std::puts("initializing shape"); std::fflush(stdout);
    if (!shape.initializeShape(0)) return 4;
    std::printf("shape loaded: nodes=%zu sequences=%zu\n",shape.nodes.size(),shape.sequences.size()); std::fflush(stdout);
    const std::size_t oldSequences=shape.sequences.size();
    const std::size_t oldRotations=shape.nodeRotations.size();
    const std::size_t oldTranslations=shape.nodeTranslations.size();
    std::puts("importing sequence"); std::fflush(stdout);
    if (!shape.importSequences(*sequenceFile)) {
        std::fprintf(stderr,"sequence import failed at offset %d, eof=%d error=%d\n",
            sequenceFile->tell(),sequenceFile->eof(),sequenceFile->error());
        if (shape.sequences.size()>oldSequences) {
            const auto& sequence=shape.sequences[oldSequences];
            std::fprintf(stderr,"sequences=%zu rotations=%zu translations=%zu objects=%zu decals=%zu triggers=%zu ground=%zu/%zu keyframes=%zu first=%d count=%d base=%d/%d/%d/%d\n",
                shape.sequences.size(),shape.nodeRotations.size(),shape.nodeTranslations.size(),
                shape.objectStates.size(),shape.decalStates.size(),shape.triggers.size(),
                shape.groundRotations.size(),shape.groundTranslations.size(),legacyKeyframeScratch.size(),
                sequence.firstGroundFrame,sequence.numGroundFrames,sequence.baseRotation,
                sequence.baseTranslation,sequence.baseObjectState,sequence.baseDecalState);
            std::uint32_t first=0; int rotations=0,objects=0,decals=0;
            for (unsigned i=0;i<TSIntegerSet::capacity;++i) {
                rotations+=sequence.rotationMatters.test(i);
                objects+=sequence.visMatters.test(i)||sequence.frameMatters.test(i)||sequence.matFrameMatters.test(i);
                decals+=sequence.decalMatters.test(i);
            }
            if (!legacyFirstKeyframeScratch.empty()) first=static_cast<std::uint32_t>(legacyFirstKeyframeScratch[0]);
            std::fprintf(stderr,"first-key=%u source=%d/%d/%d tracks=%d/%d/%d frames=%d\n",first,
                first<legacyKeyframeScratch.size()?legacyKeyframeScratch[first].first:-1,
                first<legacyKeyframeScratch.size()?legacyKeyframeScratch[first].second:-1,
                first<legacyKeyframeScratch.size()?legacyKeyframeScratch[first].third:-1,
                rotations,objects,decals,sequence.numKeyframes);
        }
        return 5;
    }
    std::puts("sequence imported"); std::fflush(stdout);
    if (shape.sequences.size()<=oldSequences || shape.nodeRotations.size()<=oldRotations ||
        shape.nodeTranslations.size()<=oldTranslations || legacyKeyframeScratch.empty()) return 6;
    if (shape.sequences.size()-oldSequences!=1 || shape.sequences[oldSequences].numKeyframes!=106 ||
        shape.sequences[oldSequences].baseRotation!=0 || shape.sequences[oldSequences].baseTranslation!=0 ||
        shape.nodeRotations.size()-oldRotations!=8374 || shape.nodeTranslations.size()-oldTranslations!=8374) return 8;
    for (const auto& sequence:shape.sequences) {
        if (sequence.numKeyframes<0 || sequence.baseRotation<0 || sequence.baseTranslation<0 ||
            sequence.firstTrigger<0 || sequence.firstTrigger+sequence.numTriggers>
                static_cast<std::int32_t>(shape.triggers.size())) return 7;
    }
    std::printf("v16 DSQ imported: sequences=%zu rotations=%zu translations=%zu triggers=%zu ground=%zu\n",
        shape.sequences.size()-oldSequences,shape.nodeRotations.size()-oldRotations,
        shape.nodeTranslations.size()-oldTranslations,shape.triggers.size(),shape.groundRotations.size());
}
