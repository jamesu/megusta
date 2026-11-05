#include "../src/dts/shape.h"
#include "../src/dts/wrappers/shape_wrappers.h"
#include "../src/engine/resource_file.h"
#include <cstdio>
#include <fstream>
#include <string>

using namespace megusta;
using namespace megusta::dts;

static bool importPair(const std::string& shapeName,const std::string& sequenceName,
                       std::size_t& sequenceTotal,std::size_t& frameTotal) {
    const std::string shapePath="fixtures/"+shapeName;
    const std::string sequencePath="fixtures/"+sequenceName;
    std::FILE* shapeStream=std::fopen(shapePath.c_str(),"rb");
    std::FILE* sequenceStream=std::fopen(sequencePath.c_str(),"rb");
    if (!shapeStream || !sequenceStream) {
        if (shapeStream) std::fclose(shapeStream);
        if (sequenceStream) std::fclose(sequenceStream);
        std::fprintf(stderr,"cannot open %s + %s\n",shapeName.c_str(),sequenceName.c_str());
        return false;
    }
    ResourceFile shapeFile(shapeStream);
    ResourceFile sequenceFile(sequenceStream);
    TSShape shape;
    if (shape.readHeader(shapeFile)!=TSShape::HeaderResult::legacyUnified ||
        !shape.readLegacyPrefix(shapeFile) || !shape.readLegacyTail(shapeFile,"") ||
        !shape.assembleLegacyMeshes(false) || !shape.initializeShape(0)) {
        std::fprintf(stderr,"shape read failed: %s\n",shapeName.c_str());
        return false;
    }
    const std::size_t sequenceBase=shape.sequences.size();
    if (!TSShape_importSequences_impl(&shape,&sequenceFile)) {
        std::fprintf(stderr,"sequence import failed: %s + %s at %d\n",shapeName.c_str(),
                     sequenceName.c_str(),sequenceFile.tell());
        return false;
    }
    for (std::size_t i=sequenceBase;i<shape.sequences.size();++i) {
        const auto& sequence=shape.sequences[i];
        if (sequence.numKeyframes<0 || sequence.baseRotation<0 || sequence.baseTranslation<0 ||
            sequence.firstTrigger<0 || sequence.firstTrigger+sequence.numTriggers>
                static_cast<std::int32_t>(shape.triggers.size())) {
            std::fprintf(stderr,"invalid imported sequence %s + %s entry %zu\n",
                         shapeName.c_str(),sequenceName.c_str(),i-sequenceBase);
            return false;
        }
        frameTotal+=static_cast<std::size_t>(sequence.numKeyframes);
    }
    sequenceTotal+=shape.sequences.size()-sequenceBase;
    return true;
}

int main(int argc,char** argv) {
    if (argc!=2) return 2;
    std::ifstream manifest(argv[1]);
    if (!manifest) return 3;
    std::size_t pairs=0,sequences=0,frames=0;
    std::string shapeName,sequenceName;
    while (manifest>>shapeName>>sequenceName) {
        if (!importPair(shapeName,sequenceName,sequences,frames)) return 4;
        ++pairs;
    }
    if (!pairs || !sequences) return 5;
    std::printf("imported %zu shipped DSQs across %zu shape pairs: %zu sequences, %zu keyframes\n",
                pairs,pairs,sequences,frames);
    return 0;
}
