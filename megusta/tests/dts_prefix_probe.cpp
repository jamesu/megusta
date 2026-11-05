#include "dts/shape.h"
#include "dts/shape_instance.h"
#include "dts/wrappers/shape_wrappers.h"
#include "engine/resource_file.h"
#include <cstdio>
#include <cstring>
#include <cmath>
int main(int argc,char** argv) {
    if (argc==2 && std::strcmp(argv[1],"--influence-check")==0) {
        megusta::dts::TSSkinMesh mesh;
        mesh.vertexIndices={0,0,0,1}; mesh.boneIndices={2,3,4,5}; mesh.weights={0.8f,0.15f,0.05f,1.0f};
        if (!mesh.reduceInfluences({2,0.1f}) || mesh.vertexIndices.size()!=3 || mesh.boneIndices[1]!=3 || mesh.boneIndices[2]!=5 ||
            std::fabs(mesh.weights[0]-0.84210526f)>0.00001f || std::fabs(mesh.weights[1]-0.15789474f)>0.00001f || mesh.weights[2]!=1.0f) return 1;
        std::puts("influence reduction sample: passed"); return 0;
    }
    const bool triangles=argc>1 && std::strcmp(argv[1],"--read-triangles")==0;
    const bool strips=argc>1 && std::strcmp(argv[1],"--read-strips")==0;
    const bool instanceCheck=argc>1 && std::strcmp(argv[1],"--instance-check")==0;
    const bool readCheck=triangles || strips || instanceCheck || (argc>1 && std::strcmp(argv[1],"--read-check")==0);
    for (int i=readCheck?2:1;i<argc;++i) {
        std::FILE* raw=std::fopen(argv[i],"rb");
        if (!raw) { std::printf("%s\topen-failed\n",argv[i]); continue; }
        megusta::ResourceFile file(raw);
        megusta::dts::TSShape shape;
        if (readCheck) {
            megusta::dts::dtsConvertTriangles=triangles;
            megusta::dts::dtsUseEncodedNormals=false;
            megusta::dts::minimumStripTriangles=strips?3:0;
            bool ok=TSShape_read_impl(&shape,&file)!=0;
            if (ok && (shape.splitWords.size()!=0 || shape.meshStreams.words32.size()!=0 ||
                       shape.meshStreams.words16.size()!=0 || shape.meshStreams.bytes8.size()!=0 ||
                       !shape.meshes.empty() || !shape.skins.empty())) return 1;
            if (ok && shape.assemblyArena.size()*sizeof(std::uint64_t)<shape.assemblyArenaBytes) return 1;
            if (instanceCheck) {
                megusta::dts::TSShapeInstance instance(&shape,0);
                std::printf("%s\t%d\t%d\t%u\t%u\t%u\n",argv[i],ok,instance.instanceLinksReady,
                    static_cast<unsigned>(instance.nodeTransforms.size()),static_cast<unsigned>(instance.meshObjects.size()),static_cast<unsigned>(instance.decalObjects.size()));
                continue;
            }
            std::printf("%s\t%d\t%d\t%u\t%u\t%u\t%u\t%u\t%u\n",argv[i],ok,file.tell(),
              static_cast<unsigned>(shape.nodes.size()),static_cast<unsigned>(shape.objects.size()),
              static_cast<unsigned>(shape.assembledMeshes.size()),static_cast<unsigned>(shape.assembledSkins.size()),
              static_cast<unsigned>(shape.defaultRotations.size()),static_cast<unsigned>(shape.sequences.size()));
            continue;
        }
        auto header=shape.readHeader(file);
        if (header!=megusta::dts::TSShape::HeaderResult::legacyUnified) { std::printf("%s\theader-failed\n",argv[i]); continue; }
        bool prefix=shape.readLegacyPrefix(file);
        auto prefixEnd=file.tell();
        bool tail=prefix && shape.readLegacyTail(file,"");
        bool assembled=tail && shape.assembleLegacyMeshes(false);
        std::size_t vertices=0,indices=0;
        for (const auto& m:shape.assembledMeshes) { vertices+=m.base.vertices.size(); indices+=m.base.indices.size(); }
        for (const auto& m:shape.assembledSkins) { vertices+=m.base.vertices.size(); indices+=m.base.indices.size(); }
        std::printf("%s\t%d\t%d\t%d\t%d\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%u\t%u\t%u\t%u\t%u\n",
          argv[i],prefix,tail,prefixEnd,file.tell(),static_cast<unsigned>(shape.nodes.size()),static_cast<unsigned>(shape.objects.size()),
          static_cast<unsigned>(shape.decals.size()),static_cast<unsigned>(shape.iflMaterials.size()),static_cast<unsigned>(shape.firstNodes.size()),
          static_cast<unsigned>(shape.legacyKeyframes.size()),static_cast<unsigned>(shape.nodeStates.size()),static_cast<unsigned>(shape.objectStates.size()),
          static_cast<unsigned>(shape.decalStates.size()),static_cast<unsigned>(shape.triggers.size()),static_cast<unsigned>(shape.details.size()),static_cast<unsigned>(shape.sequences.size()),assembled,static_cast<unsigned>(shape.legacyGuardMismatches),
          static_cast<unsigned>(shape.assembledMeshes.size()),static_cast<unsigned>(shape.assembledSkins.size()),static_cast<unsigned>(vertices),static_cast<unsigned>(indices));
    }
}
