#include "legacy_mesh.h"
#include "legacy_alloc.h"
#include "sequence.h"
#include "../engine/resource_file.h"
#include <cstdint>
namespace megusta { namespace dts {
bool readLegacyMesh(ResourceFile& file,DTSLegacyAlloc& out,std::uint32_t version,DTSMeshKind type) {
    if (type==DTSMeshNull) return true;
    // These reads intentionally preserve the saved parser's unchecked counts
    // and unchecked ResourceFile reads. In particular, the later type-1
    // payload overwrites a previously reserved word range.
    auto readAppend=[&]() { return out.readAppend32(file); };
    auto readStore=[&](std::uint32_t& value) { out.readStore32(file,value); };
    auto readWords=[&](std::uint32_t count) { out.read32(file,static_cast<int>(count)); };
    auto readShorts=[&](std::uint32_t count) { out.read16(file,static_cast<int>(count)); };
    auto readSizedWords=[&](std::uint32_t width) { const auto count=readAppend(); readWords(count*width); };
    out.guard();
    readWords(2);
    *out.alloc32(1)=0xffffffffu; out.alloc32(10);
    readSizedWords(3); readSizedWords(2);
    std::uint32_t n=0;
    readStore(n); readWords(n*3);
    n=readAppend();
    for (std::uint32_t i=0;i<n;++i) {
        if (version<18) {
            std::uint32_t first,count;
            readStore(first); readStore(count);
            auto* p=out.alloc16(2); p[0]=static_cast<std::uint16_t>(first); p[1]=static_cast<std::uint16_t>(count);
        } else readShorts(2);
        readWords(1);
    }
    n=readAppend();
    if (version<18) {
        auto* p=out.alloc16(n);
        for (std::uint32_t i=0;i<n;++i) { std::uint32_t value; readStore(value); p[i]=static_cast<std::uint16_t>(value); }
    } else readShorts(n);
    *out.alloc32(1)=0;
    readWords(2);
    out.guard();
    if (type==DTSMeshSkin) {
        readSizedWords(3);
        readStore(n); readWords(n*3);
        readSizedWords(16); readSizedWords(1);
        readStore(n); readWords(n);
        auto index=out.words32.size(); out.alloc32(n);
        readSizedWords(1);
        readStore(n);
        file.read(out.words32.data()+index,4,n);
        out.guard();
    } else if (type==DTSMeshSorted) {
        readSizedWords(8); readSizedWords(1); readSizedWords(1); readSizedWords(1); readSizedWords(1);
        std::uint8_t flag; file.read(&flag,1,1);
        *out.alloc32(1)=flag!=0; out.guard();
    } else if (type==DTSMeshDecal) {
        readSizedWords(1);
        if (version>16) {
            for (int i=0;i<2;++i) {
                readStore(n);
                for (std::uint32_t j=0;j<n;++j) { std::uint32_t ignored; readStore(ignored); }
            }
        }
        n=readAppend();
        out.guard();
    }
    return true;
}
} }

