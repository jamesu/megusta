#include "../src/dts/shape.h"
#include "../src/engine/resource_file.h"
#include <cstdint>
#include <cstring>
#include <vector>

using namespace megusta;
using namespace megusta::dts;

namespace {
struct Bytes {
    std::vector<std::uint8_t> data;
    void word(std::uint32_t value) {
        const auto* bytes=reinterpret_cast<const std::uint8_t*>(&value);
        data.insert(data.end(),bytes,bytes+4);
    }
    void shortWord(std::int16_t value) {
        const auto* bytes=reinterpret_cast<const std::uint8_t*>(&value);
        data.insert(data.end(),bytes,bytes+2);
    }
    void real(float value) {
        std::uint32_t bits=0; std::memcpy(&bits,&value,4); word(bits);
    }
    void name(const char* value) {
        const auto length=static_cast<std::uint32_t>(std::strlen(value));
        word(length); data.insert(data.end(),value,value+length);
    }
    void quat(std::int16_t x,std::int16_t y,std::int16_t z,std::int16_t w) {
        shortWord(x); shortWord(y); shortWord(z); shortWord(w);
    }
    void point(float x,float y,float z) { real(x); real(y); real(z); }
    void integerSet(bool rootMatters) {
        word(0); word(TSIntegerSet::wordCount);
        word(rootMatters ? 1u : 0u);
        for (unsigned i=1;i<TSIntegerSet::wordCount;++i) word(0);
    }
};

static Bytes makeSequence(std::uint32_t version) {
    Bytes out;
    out.word(version);
    out.word(1);                 // one source node name
    out.name("root");
    out.word(0); out.word(0);    // legacy node/object state bases
    if (version<17) {
        out.word(0);             // no legacy first-keyframe records
    }
    if (version<22) {
        out.word(1);             // one interleaved quaternion/translation state
        out.quat(11,12,13,14);
        out.point(1.0f,2.0f,3.0f);
    } else {
        out.word(1); out.quat(11,12,13,14); // rotations
        out.word(1); out.point(1.0f,2.0f,3.0f); // translations
        out.word(1); out.real(2.0f);          // uniform scales
        out.word(1); out.point(4.0f,5.0f,6.0f); // aligned scales
        out.word(1); out.quat(21,22,23,24);   // arbitrary-scale rotations
        out.point(7.0f,8.0f,9.0f); // factors share the arbitrary-rotation count
        out.word(1); out.point(10.0f,11.0f,12.0f); // ground translations
        out.quat(31,32,33,34);   // rotations share the ground-translation count
    }
    out.word(0); out.word(1);    // object-state base, one sequence
    out.name("walk");
    if (version>=22) out.word(1); // uniform-scale animation flag
    out.word(2); out.real(1.0f); // keyframes, duration
    if (version<22) { out.data.push_back(0); out.data.push_back(0); out.data.push_back(0); }
    out.word(5); out.word(version<22 ? 1u : 0u); out.word(1); // priority, ground start/count
    out.word(version<22 ? 1u : 0u);     // pre-v22 indices include incoming default nodes
    if (version>=22) out.word(0);      // base translation
    if (version>=22) out.word(0);      // base scale
    out.word(0); out.word(0);          // base object/decal state
    out.word(0); out.word(0);          // first trigger/count
    out.real(0.0f);                    // tool begin
    out.integerSet(true);              // rotation matters: root
    if (version>=22) {
        out.integerSet(true);          // translation matters: root
        out.integerSet(true);          // scale matters: root
    }
    if (version>10) out.integerSet(false); // decal matters
    if (version>5) out.integerSet(false);  // IFL matters
    out.integerSet(false);             // visibility matters
    out.integerSet(false);             // frame matters
    out.integerSet(false);             // material-frame matters
    out.word(0);                       // trigger count
    return out;
}

static bool checkVersion(std::uint32_t version) {
    TSShape shape;
    shape.nodes.resize(1); shape.nodes[0].nameIndex=0;
    shape.names.push_back("root");
    shape.nodeRotations.push_back({1,2,3,4});
    shape.nodeTranslations.push_back({-1.0f,-2.0f,-3.0f});
    if (version>=22) {
        shape.nodeUniformScales.push_back(0.5f);
        shape.nodeAlignedScales.push_back({0.5f,0.6f,0.7f});
        shape.nodeArbitraryScaleRotations.push_back({5,6,7,8});
        shape.nodeArbitraryScaleFactors.push_back({0.8f,0.9f,1.0f});
        shape.groundTranslations.push_back({-4.0f,-5.0f,-6.0f});
        shape.groundRotations.push_back({9,10,11,12});
    }
    Bytes input=makeSequence(version);
    ResourceFile file(input.data.data(),static_cast<std::int32_t>(input.data.size()));
    if (!shape.importSequences(file)) return false;
    if (file.tell()!=static_cast<std::int32_t>(input.data.size()) || shape.sequences.size()!=1) return false;
    const auto& sequence=shape.sequences[0];
    if (sequence.nameIndex<0 || sequence.numKeyframes!=2 || sequence.baseRotation!=1 ||
        sequence.baseTranslation!=1 || !sequence.rotationMatters.test(0) ||
        !sequence.translationMatters.test(0) ||
        sequence.firstGroundFrame!=(version>=22 ? 1 : 0) ||
        sequence.numGroundFrames!=1) return false;
    if (shape.nodeRotations.size()!=2 || shape.nodeTranslations.size()!=2 ||
        shape.nodeRotations[1].x!=11 || shape.nodeTranslations[1].x!=1.0f) return false;
    if (version<22) {
        if (shape.groundRotations.size()!=1 || shape.groundRotations[0].x!=11 ||
            shape.groundTranslations.size()!=1 || shape.groundTranslations[0].x!=1.0f) return false;
    } else {
        if (sequence.baseScale!=1 || !sequence.scaleMatters.test(0) ||
            shape.nodeUniformScales.size()!=2 || shape.nodeUniformScales[1]!=2.0f ||
            shape.groundRotations.size()!=2 || shape.groundRotations[1].x!=31 ||
            shape.groundTranslations.size()!=2 || shape.groundTranslations[1].x!=10.0f) return false;
    }
    return true;
}
}

int main() {
    return checkVersion(17) && checkVersion(21) && checkVersion(22) ? 0 : 1;
}
