#include "mesh.h"
#include "shape_alloc.h"
#include "material_list.h"
#include "shape.h"
#include "../engine/opengl_adapter.h"
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <limits>
namespace megusta { namespace dts {
DTSMeshAssemblyContext* activeMeshAssembly=nullptr;
std::vector<std::int32_t> shadowCoordinates;
TSMeshPoint normalDecodeTable[256]={};
std::vector<TSMeshPoint> decodedNormalScratch;
namespace {
// Literal vectors recovered from the initializer at 004c2de0. The first 110
// entries were emitted as IEEE-754 words; the remainder came from decimal
// constants. These are data, not a regenerated spherical distribution.
constexpr TSMeshPoint normalDecodeValues[256]={
{0.565060973f,-0.270644009f,-0.779395998f},{-0.309803993f,-0.73111397f,0.607860029f},{-0.867411971f,0.472956985f,0.154618993f},{-0.757488012f,0.498187989f,-0.421925008f},
{0.306834012f,-0.915340006f,0.26077801f},{0.0987540036f,0.639153004f,-0.762713015f},{0.713706017f,-0.558861971f,-0.422251999f},{-0.890430987f,-0.407602996f,-0.202465996f},
{0.848049998f,-0.487612009f,-0.207475007f},{-0.232225999f,0.776854992f,0.585292995f},{-0.940195024f,0.30449f,-0.152705997f},{0.602019012f,-0.491878003f,-0.628991008f},
{-0.0968350023f,-0.49435401f,-0.863849998f},{0.0266299993f,-0.323659003f,-0.945798993f},{0.0192079991f,0.909385979f,0.415509999f},{0.854439974f,0.491730005f,0.167731002f},
{-0.418835014f,0.866521001f,-0.271512002f},{0.465023994f,0.409666985f,0.784808993f},{-0.674390972f,-0.691087008f,-0.259992003f},{0.303858012f,-0.869270027f,-0.389921993f},
{0.991333008f,0.0900610015f,-0.0956400037f},{-0.275923997f,-0.36954999f,0.887297988f},{0.426544994f,-0.465961993f,0.775201976f},{-0.482740998f,-0.873278022f,-0.0659200028f},
{0.0636160001f,0.932012022f,-0.35679999f},{0.624786019f,-0.0613150001f,0.778384984f},{-0.530300021f,0.416850001f,0.738252997f},{0.312144011f,-0.757027984f,-0.573998988f},
{0.399287999f,-0.587091029f,-0.70419699f},{-0.132697999f,0.482876986f,0.865576029f},{0.950966001f,0.306529999f,0.0412679985f},{-0.0159230009f,-0.144299999f,0.98940599f},
{-0.407521993f,-0.854192972f,0.322925001f},{-0.932398021f,0.220464006f,0.286408007f},{0.477508992f,0.87658f,0.0599359982f},{0.33713299f,0.932605982f,-0.128795996f},
{-0.638117015f,0.199338004f,0.743686974f},{-0.677453995f,0.445349008f,0.585422993f},{-0.446714997f,0.889059007f,-0.100098997f},{-0.410023987f,0.909168005f,0.0727590024f},
{0.708462f,0.702103019f,-0.0716409981f},{-0.0488010012f,-0.903683007f,-0.425410986f},{-0.513680995f,-0.646901011f,0.563606024f},{-0.0800219998f,0.000676000025f,-0.996792972f},
{0.0669659972f,-0.991150022f,-0.114615001f},{-0.245220006f,0.639317989f,-0.728793025f},{0.250977993f,0.855979025f,0.452006012f},{-0.123547003f,0.982442975f,-0.139790997f},
{-0.794825017f,0.0302540008f,-0.606083989f},{-0.772904992f,0.547941029f,0.319967002f},{0.916347027f,0.369614005f,-0.153927997f},{-0.388202995f,0.105394997f,0.915526986f},
{-0.700468004f,-0.709334016f,0.0786769986f},{-0.816192985f,0.390455008f,0.425880015f},{-0.0430070013f,0.769222021f,-0.637533009f},{0.911444008f,0.113150001f,0.395559996f},
{0.845800996f,0.156091005f,-0.510152996f},{0.829801023f,-0.0293400008f,0.557286978f},{0.259528995f,0.416263014f,0.871417999f},{0.231128007f,-0.845982015f,0.480515003f},
{-0.626203001f,-0.646167994f,0.436277002f},{-0.197046995f,-0.0657910034f,0.978183985f},{-0.255692005f,-0.637488008f,-0.726794004f},{0.530662f,-0.844385028f,-0.073567003f},
{-0.779887021f,0.617066979f,-0.104898997f},{0.73990798f,0.113984004f,0.662981987f},{-0.218801007f,0.93019402f,-0.294728994f},{-0.374231011f,0.818665981f,0.435588986f},
{-0.72025001f,-0.0282850005f,0.69313699f},{0.0753889978f,0.415048987f,0.906669974f},{-0.539723992f,-0.106619999f,0.835062981f},{-0.452612013f,-0.754669011f,-0.474990994f},
{0.682821989f,0.581233978f,-0.442629009f},{0.00243499991f,-0.618462026f,-0.785811007f},{-0.39763099f,0.110766001f,-0.910835028f},{0.133935004f,-0.985437989f,0.104754001f},
{0.759097993f,-0.608003974f,0.232594997f},{-0.825239003f,-0.256087005f,0.503387988f},{0.101692997f,-0.56556797f,0.818408012f},{0.386377007f,0.793546021f,-0.470104009f},
{-0.520515978f,-0.840690017f,0.149345994f},{-0.784548998f,-0.479672015f,0.392935008f},{-0.325322002f,-0.927581012f,-0.183734998f},{-0.0692939982f,-0.428541005f,0.900861025f},
{0.993354023f,-0.115023002f,-0.00428800005f},{-0.123896003f,-0.70056802f,0.702746987f},{-0.438030988f,-0.12088f,-0.890794992f},{0.0633139983f,0.813233018f,0.578483999f},
{0.322044998f,0.889086008f,-0.325289011f},{-0.133521006f,0.875063002f,-0.465227991f},{0.637154996f,0.564813972f,0.52442199f},{0.26009199f,-0.669353008f,0.695930004f},
{0.953194976f,0.0404849984f,-0.29963401f},{-0.840664983f,-0.0765089989f,0.536123991f},{-0.971350014f,0.202093005f,0.125046998f},{-0.804306984f,-0.396311998f,-0.442748994f},
{-0.936746001f,0.0695720017f,0.343026996f},{0.426544994f,-0.465961993f,0.775201976f},{0.794542015f,-0.227449998f,0.563000023f},{-0.892171979f,0.0911689997f,-0.442398995f},
{-0.312653989f,0.541263998f,0.78056401f},{0.590602994f,-0.735617995f,-0.331743002f},{-0.0980399996f,-0.986712992f,0.129557997f},{0.569646001f,0.283078015f,-0.771602988f},
{0.431050986f,-0.407384992f,-0.805128992f},{-0.162086993f,-0.938749015f,-0.304104f},{0.241532996f,-0.359508991f,0.901341021f},{-0.576191008f,0.614938974f,0.538380027f},
{-0.0251100007f,0.08574f,0.996001005f},{-0.352692991f,-0.198167995f,0.914515018f},{-0.604577f,0.700711f,0.378802f},{0.465024f,0.409667f,0.784809f},
{-0.254684f,-0.030474f,-0.966544f},{-0.604789f,0.791809f,0.085259f},{-0.705147f,-0.399298f,0.585943f},{0.185691f,0.017236f,-0.982457f},
{0.044588f,0.973094f,0.226052f},{-0.405463f,0.642367f,0.650357f},{-0.563959f,0.599136f,-0.568319f},{0.367162f,-0.072253f,-0.927347f},
{0.960429f,-0.21357f,-0.178783f},{-0.192629f,0.906005f,0.376893f},{-0.199718f,-0.359865f,-0.911378f},{0.485072f,0.121233f,-0.86603f},
{0.467163f,-0.874294f,0.131792f},{-0.638953f,-0.716603f,0.279677f},{-0.62271f,0.047813f,-0.78099f},{0.828724f,-0.054433f,-0.557004f},
{0.130241f,0.99108f,0.028245f},{0.310995f,-0.950076f,-0.025242f},{0.818118f,0.275336f,0.50485f},{0.676328f,0.387023f,0.626733f},
{-0.100433f,0.495114f,-0.863004f},{-0.949609f,-0.240681f,-0.200786f},{-0.10261f,0.261831f,-0.959644f},{-0.845732f,-0.493136f,0.20385f},
{0.672617f,-0.738838f,0.04129f},{0.380465f,0.875938f,0.296613f},{-0.811223f,0.262027f,-0.522742f},{-0.074423f,-0.77567f,-0.626736f},
{-0.286499f,0.75585f,-0.588735f},{0.291182f,-0.276189f,-0.915933f},{-0.638117f,0.199338f,0.743687f},{0.439922f,-0.864433f,-0.243359f},
{0.177649f,0.206919f,0.962094f},{0.277107f,0.948521f,0.153361f},{0.507629f,0.661918f,-0.551523f},{-0.50311f,-0.579308f,-0.641313f},
{0.600522f,0.736495f,-0.311364f},{-0.691096f,-0.715301f,-0.103592f},{-0.041083f,-0.858497f,0.511171f},{0.207773f,-0.480062f,-0.852274f},
{0.795719f,0.464614f,0.388543f},{-0.100433f,0.495114f,-0.863004f},{0.703249f,0.065157f,-0.707951f},{-0.324171f,-0.941112f,0.096024f},
{-0.134933f,-0.940212f,0.312722f},{-0.43824f,0.752088f,-0.492249f},{0.964762f,-0.198855f,0.172311f},{-0.831799f,0.196807f,0.519015f},
{-0.508008f,0.819902f,0.263986f},{0.471075f,-0.001146f,0.882092f},{0.919512f,0.246162f,-0.306435f},{-0.96005f,0.279828f,-0.001187f},
{0.110232f,-0.847535f,-0.519165f},{0.208229f,0.69736f,0.685806f},{-0.19968f,-0.560621f,0.803637f},{0.170135f,-0.679985f,-0.713214f},
{0.758371f,-0.494907f,0.424195f},{0.077734f,-0.755978f,0.649965f},{0.612831f,-0.672475f,0.414987f},{0.142776f,0.836698f,-0.528726f},
{-0.765185f,0.635778f,0.101382f},{0.669873f,-0.419737f,0.612447f},{0.593549f,0.194879f,0.780847f},{0.64693f,0.752173f,0.125368f},
{0.837721f,0.545266f,-0.030127f},{0.541505f,0.76807f,0.34182f},{0.760679f,-0.365715f,-0.536301f},{0.381516f,0.640377f,0.666605f},
{0.565794f,-0.072415f,-0.821361f},{-0.466072f,-0.401588f,0.788356f},{0.987146f,0.09629f,0.12756f},{0.509709f,-0.688886f,-0.515396f},
{-0.135132f,-0.988046f,-0.074192f},{0.600499f,0.476471f,-0.642166f},{-0.732326f,-0.27532f,-0.622815f},{-0.881141f,-0.470404f,0.048078f},
{0.051548f,0.601042f,0.797553f},{0.402027f,-0.763183f,0.505891f},{0.404233f,-0.208288f,0.890624f},{-0.311793f,0.343843f,0.885752f},
{0.098132f,-0.937014f,0.335223f},{0.537158f,0.830585f,-0.146936f},{0.725277f,0.298172f,-0.620538f},{-0.882025f,0.342976f,-0.32311f},
{-0.668829f,0.424296f,-0.610443f},{-0.408835f,-0.476442f,-0.778368f},{0.809472f,0.397249f,-0.432375f},{-0.909184f,-0.205938f,-0.361903f},
{0.86693f,-0.347934f,-0.356895f},{0.91166f,-0.141281f,-0.385897f},{-0.431404f,-0.844074f,-0.31848f},{-0.950593f,-0.073496f,0.301614f},
{-0.719716f,0.626915f,-0.298305f},{-0.779887f,0.617067f,-0.104899f},{-0.475899f,-0.54263f,0.692151f},{0.081952f,-0.157248f,-0.984153f},
{0.92399f,-0.381662f,-0.024025f},{-0.957998f,0.120979f,-0.260008f},{0.306601f,0.227975f,-0.924134f},{-0.141244f,0.989182f,0.039601f},
{0.077097f,0.186288f,-0.979466f},{-0.630407f,-0.259801f,0.731499f},{0.71815f,0.637408f,0.279233f},{0.340946f,0.110494f,0.933567f},
{-0.396671f,0.50302f,-0.767869f},{0.636943f,-0.245005f,0.730942f},{-0.849605f,-0.51866f,-0.095724f},{-0.388203f,0.105395f,0.915527f},
{-0.280671f,-0.776541f,-0.564099f},{-0.60168f,0.215451f,-0.769131f},{-0.660112f,-0.632371f,-0.405412f},{0.921096f,0.284072f,0.266242f},
{0.07485f,-0.300846f,0.950731f},{0.943952f,-0.067062f,0.323198f},{-0.917838f,-0.254589f,0.304561f},{0.889843f,-0.409008f,0.202219f},
{-0.565849f,0.753721f,-0.334246f},{0.79146f,0.555918f,-0.25406f},{0.261936f,0.70359f,-0.660568f},{-0.234406f,0.952084f,0.196444f},
{0.111205f,0.979492f,-0.168014f},{-0.869844f,-0.109095f,-0.481113f},{-0.337728f,-0.269701f,-0.901777f},{0.366793f,0.408875f,-0.835634f},
{-0.098749f,0.261316f,0.960189f},{-0.272379f,-0.8471f,0.456324f},{-0.319506f,0.287444f,-0.902935f},{0.873383f,-0.294109f,0.388203f},
{-0.08895f,0.71045f,0.698104f},{0.551238f,-0.786552f,0.27834f},{0.724436f,-0.663575f,-0.186712f},{0.529741f,-0.606539f,0.592861f},
{-0.949743f,-0.282514f,0.134809f},{0.155047f,0.419442f,-0.894443f},{-0.562653f,-0.329139f,-0.758346f},{0.816407f,-0.576953f,0.024576f},
{0.17855f,-0.950242f,-0.255266f},{0.479571f,0.706691f,0.520192f},{0.391687f,0.559884f,-0.730145f},{0.724872f,-0.20557f,-0.657496f},
{-0.663196f,-0.517587f,-0.540624f},{-0.660054f,-0.122486f,-0.741165f},{-0.531989f,0.374711f,-0.759328f},{0.194979f,-0.05912f,0.979024f}
};
}
void initializeNormalDecodeTable() {
    std::copy(normalDecodeValues,normalDecodeValues+256,normalDecodeTable);
}
extern "C" void MEGUSTA_DTS_CDECL DTS_registeredStartup_004c2dd0(void) {
    initializeNormalDecodeTable();
}
const TSMeshPoint* TSMesh::getNormals(int vertexOffset) const {
    if (!(meshType&0x10000000u)) return normalData()+vertexOffset;
    // The original scratch vector reserves for vertsPerFrame, then sets its
    // logical size from encodedNormsCount before decoding that many entries.
    // Keep those two counts distinct: malformed or unusual shapes can have a
    // decoded count larger than the nominal frame size, and the loop below
    // follows the encoded stream count exactly.
    if (decodedNormalScratch.capacity()<static_cast<std::size_t>(verticesPerFrame))
        decodedNormalScratch.reserve(static_cast<std::size_t>(verticesPerFrame));
    decodedNormalScratch.resize(encodedNormals.size());
    for (std::size_t i=0;i<encodedNormals.size();++i)
        decodedNormalScratch[i]=normalDecodeTable[encodedNormals[static_cast<std::size_t>(vertexOffset)+i]];
    return decodedNormalScratch.data();
}
void shadowTransform(const float* matrix,const TSMeshPoint* begin,const TSMeshPoint* end,int maximum) {
    auto* output=shadowCoordinates.data();
    // The saved body loads these coefficients before its first output store.
    const float xCoefficients[4]={matrix[0],matrix[1],matrix[2],matrix[3]};
    const float yCoefficients[4]={matrix[8],matrix[9],matrix[10],matrix[11]};
    const auto project=[maximum](long double value) {
        // The saved code stores __ftol's 64-bit result into a 32-bit local
        // before applying either clamp. Preserve that narrowing/wrap step;
        // clamping the 64-bit value directly changes large coordinates.
        // VC6 __ftol returns the x87 integer-indefinite result for an invalid
        // conversion. Express it explicitly rather than invoking C++ float-
        // to-integer undefined behavior for NaN, infinity or overflow.
        constexpr long double limit=9223372036854775808.0L;
        const std::int64_t converted=(value>=-limit && value<limit)
            ? static_cast<std::int64_t>(value) : std::numeric_limits<std::int64_t>::min();
        const std::uint32_t lowBits=static_cast<std::uint32_t>(converted);
        std::int32_t coordinate;
        std::memcpy(&coordinate,&lowBits,sizeof(coordinate));
        if (coordinate<0) return std::int32_t{0};
        if (maximum<coordinate) return static_cast<std::int32_t>(maximum);
        return coordinate;
    };
    while (begin<end) {
        // The saved body calls VC6 __ftol for both projected coordinates;
        // it narrows that result to 32 bits before clamping.
        output[0]=project(static_cast<long double>(xCoefficients[0])*begin->x+
            static_cast<long double>(xCoefficients[1])*begin->y+static_cast<long double>(xCoefficients[2])*begin->z+xCoefficients[3]);
        output[1]=project(static_cast<long double>(yCoefficients[0])*begin->x+
            static_cast<long double>(yCoefficients[1])*begin->y+static_cast<long double>(yCoefficients[2])*begin->z+yCoefficients[3]);
        output+=2; ++begin;
    }
}
namespace {
bool convertAssemblyPrimitives(TSMesh& mesh,const DTSMeshAssemblyContext& context,bool sorted) {
    // 004d4b10 clears both conversion-mode globals around base assembly.
    // Sorted meshes therefore always take the multiple-strip path.
    if (sorted) return mesh.leaveAsMultipleStrips(minimumStripTriangles);
    if (context.convertTriangles) return mesh.convertToTriangles();
    // 004c5c90 selects the single-strip converter from dtsPrimitiveGlModes[3]
    // after the triangle-conversion global has been checked.
    if (static_cast<std::uint8_t>(dtsPrimitiveModes[3])!=0) {
        const int minimum=context.minimumStripTriangles>=0 ? context.minimumStripTriangles : minimumStripTriangles;
        return mesh.convertToSingleStrip(minimum);
    }
    if (context.minimumStripTriangles>=0) return mesh.leaveAsMultipleStrips(context.minimumStripTriangles);
    return mesh.leaveAsMultipleStrips(minimumStripTriangles);
}
void* sharedMeshData32Internal(int parent,int count,void** parentData,char skip) {
    auto& context=*activeMeshAssembly;
    auto& allocator=*context.allocator;
    if (parent<0) return skip ? const_cast<void*>(allocator.getPointer32(count)) : allocator.copyToShape32(count,false);
    void* source=parentData[parent];
    if (context.retainedMeshes[parent] || skip) return source;
    void* destination=allocator.allocShape32(count);
    if (destination && source) std::memcpy(destination,source,static_cast<std::size_t>(count)*4);
    return destination;
}
void* sharedMeshData8Internal(int parent,int count,void** parentData,char skip) {
    auto& context=*activeMeshAssembly;
    auto& allocator=*context.allocator;
    if (parent<0) return skip ? const_cast<void*>(allocator.getPointer8(count)) : allocator.copyToShape8(count,false);
    void* source=parentData[parent];
    if (context.retainedMeshes[parent] || skip) return source;
    void* destination=allocator.allocShape8(count);
    // The saved helper allocates count bytes but copies count*4. Keep the
    // recovered operation here; the normalized arena must preserve this
    // source behavior until encoded-normal ownership is resolved.
    if (destination && source) std::memcpy(destination,source,static_cast<std::size_t>(count)*4);
    return destination;
}
}

void* sharedMeshData32(int parent,int count,void** parentData,char skip) {
    return sharedMeshData32Internal(parent,count,parentData,skip);
}
void* sharedMeshData8(int parent,int count,void** parentData,char skip) {
    return sharedMeshData8Internal(parent,count,parentData,skip);
}

void* TSMesh::assembleMesh(DTSMeshKind type,unsigned char skip) {
    auto& context=*activeMeshAssembly;
    auto& mesh=*context.currentMesh;
    // 004c5800 reserves the subtype object before setting the payload skip
    // multiplier, including for scratch/omitted meshes. Native C++ objects
    // remain in the normalized mesh records; preserve these arena ranges as
    // recovered object storage rather than silently dropping the allocation.
    constexpr int baseObjectWords=0x20,skinObjectWords=0x37;
    constexpr int decalObjectWords=0x0e,sortedObjectWords=0x2b;
    int objectWords=0;
    switch (type) {
    case DTSMeshStandard: objectWords=baseObjectWords; break;
    case DTSMeshSkin: objectWords=skinObjectWords; break;
    case DTSMeshDecal: objectWords=decalObjectWords; break;
    case DTSMeshSorted: objectWords=sortedObjectWords; break;
    default: break;
    }
    if (objectWords) context.allocator->allocShape32(objectWords);
    context.allocator->multiplier=skip ? 0u : 1u;
    struct RestoreMultiplier {
        TSShapeAlloc& allocator;
        ~RestoreMultiplier() { allocator.multiplier=1; }
    } restore{*context.allocator};
    switch (type) {
    case DTSMeshStandard: mesh.base.assemble(static_cast<char>(skip)); return &mesh.base;
    case DTSMeshSkin:
        mesh.skin.assemble(skip);
        if (context.succeeded) mesh.base=mesh.skin;
        return &mesh.skin;
    case DTSMeshDecal: mesh.decal.assemble(); return &mesh.decal;
    case DTSMeshSorted:
        mesh.sorted.assemble(static_cast<char>(skip));
        if (context.succeeded) mesh.base=mesh.sorted;
        return &mesh.sorted;
    case DTSMeshNull: context.succeeded=true; return nullptr;
    default: context.succeeded=false; return nullptr;
    }
}
void* TSMesh::getSharedData32(int parent,int count,void** parentData,char skip) {
    return sharedMeshData32Internal(parent,count,parentData,skip);
}
void* TSMesh::getSharedData8(int parent,int count,void** parentData,char skip) {
    return sharedMeshData8Internal(parent,count,parentData,skip);
}
void TSMesh::assemble(char skip) {
    auto& context=*activeMeshAssembly;
    // The saved entry receives this byte directly; retain it in the assembly
    // context for shared-data and retained-parent decisions.
    context.skip=skip!=0;
    context.succeeded=assembleBase(*context.allocator,context.version,context.encodedNormals,*context.previousMeshes);
}
void TSSortedMesh::assemble(char skip) {
    auto& context=*activeMeshAssembly;
    context.skip=skip!=0;
    context.succeeded=assemble(*context.allocator,context.version,context.encodedNormals,*context.previousMeshes);
}
void TSSkinMesh::assemble(int skip) {
    auto& context=*activeMeshAssembly;
    context.skip=static_cast<unsigned char>(skip)!=0;
    context.succeeded=assemble(*context.allocator,context.version,context.encodedNormals,*context.previousMeshes,*context.previousSkins);
}
void TSDecalMesh::assemble() {
    auto& context=*activeMeshAssembly;
    context.succeeded=assemble(*context.allocator,context.version);
}
void computeVertexBounds(const TSMeshPoint* points,int count,const float* matrix,float* box,float* center,float* radius) {
    if (!count) {
        for (int i=0;i<6;++i) box[i]=0;
        if (center) for (int i=0;i<3;++i) center[i]=0;
        if (radius) *radius=0;
        return;
    }
    auto transform=[&](const TSMeshPoint& p) {
        const float input[3]={p.x,p.y,p.z}; float output[3];
        transformPoint3(matrix,input,output);
        return TSMeshPoint{output[0],output[1],output[2]};
    };
    auto first=transform(points[0]);
    box[0]=box[3]=first.x; box[1]=box[4]=first.y; box[2]=box[5]=first.z;
    for (int i=0;i<count;++i) {
        auto p=transform(points[i]); float v[3]={p.x,p.y,p.z};
        for (int j=0;j<3;++j) {
            // Preserve the saved comparison forms, including unordered values.
            if (!(v[j]<=box[j+3])) box[j+3]=v[j];
            if (!(box[j]<=v[j])) box[j]=v[j];
        }
    }
    float localCenter[3]; if (!center) center=localCenter;
    for (int i=0;i<3;++i) center[i]=(box[i+3]+box[i])*0.5f;
    if (radius) {
        float maximum=0;
        for (int i=0;i<count;++i) {
            auto p=transform(points[i]); float x=p.x-center[0],y=p.y-center[1],z=p.z-center[2];
            float distance=x*x+y*y+z*z;
            if (distance<maximum) distance=maximum;
            maximum=distance;
        }
        // The saved helper returns x87 extended precision from SQRT(float);
        // round to the stored float only at the radius assignment.
        *radius=static_cast<float>(std::sqrt(static_cast<long double>(maximum)));
    }
}
bool TSMesh::computeBoundsFrame(const float* matrix,int frame,float* box,float* resultCenter,float* resultRadius) const {
    if (!matrix || !box) return false;
    const std::size_t available=static_cast<std::size_t>(vertexCount());
    std::size_t first=0,count=available;
    if (frame>=0) {
        if (verticesPerFrame<0) return false;
        count=static_cast<std::size_t>(verticesPerFrame);
        if (count && static_cast<std::size_t>(frame)>available/count) return false;
        first=static_cast<std::size_t>(frame)*count;
        if (first>available || count>available-first) return false;
    }
    computeBounds(matrix,box,frame,resultCenter,resultRadius); return true;
}
void TSMesh::computeBounds(const float* matrix,float* box,int frame,float* resultCenter,float* resultRadius) const {
    if (frame<0) computeVertexBounds(vertexData(),vertexCount(),matrix,box,resultCenter,resultRadius);
    else computeVertexBounds(vertexData()+verticesPerFrame*frame,verticesPerFrame,matrix,box,resultCenter,resultRadius);
}
void TSMesh::computeBounds() {
    const float identity[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    computeBounds(identity,bounds,-1,center,&radius);
}
void TSSkinMesh::computeBounds(const float* matrix,float* box,int,float* resultCenter,float* resultRadius) const {
    computeVertexBounds(initialVertices.data(),static_cast<int>(initialVertices.size()),matrix,box,resultCenter,resultRadius);
}
void TSSkinMesh::computeBounds() {
    const float identity[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    computeBounds(identity,bounds,-1,center,&radius);
}
int TSMesh::getNumPolys() const {
    int count=0;
    for (const auto& primitive:primitives) {
        int elements=static_cast<std::int16_t>(primitive.count);
        count+=(drawPrimitiveType(primitive.material)!=TSDrawPrimitiveType::TriangleList)
            ? elements-2 : elements/3;
    }
    return count;
}
int TSDecalMesh::getNumPolys() const {
    int count=0;
    for (const auto& primitive:primitives) {
        const int elements=static_cast<std::int16_t>(primitive.count);
        count+=(drawPrimitiveType(primitive.material)!=TSDrawPrimitiveType::TriangleList)
            ? elements-2 : elements/3;
    }
    return count;
}
namespace {
template<class Append>
void unwindStripCore(const std::uint16_t* indices,int count,Append append) {
    if (count<3) return;
    std::uint16_t first=indices[0],second=indices[1];
    for (int vertex=2;vertex<count;++vertex) {
        const std::uint16_t third=indices[vertex];
        std::uint16_t a=first,b=second;
        if (vertex&1) { const std::uint16_t swap=a; a=b; b=swap; }
        if (a!=b && b!=third && third!=a) {
            append(a);
            append(b);
            append(third);
        }
        first=second;
        second=third;
    }
}
template<class Emit>
void emitTriangleList(const std::uint16_t* source,unsigned start,std::uint16_t elementCount,
                      Emit emit) {
    // The saved loops promote the 16-bit count to a zero-extended 32-bit
    // value, then use a nonzero do/while loop. Counts 1 and 2 still read one
    // complete triangle; larger counts retain their unsigned value.
    if (!elementCount) return;
    int cursor=0;
    do {
        emit(source[start+static_cast<unsigned>(cursor)],
             source[start+static_cast<unsigned>(cursor)+1],
             source[start+static_cast<unsigned>(cursor)+2]);
        cursor+=3;
    } while (cursor<static_cast<int>(elementCount));
}
std::size_t requiredPrimitiveInput(const TSDrawPrimitive& primitive,int shortStripThreshold) {
    const std::size_t count=primitive.count;
    if (drawPrimitiveType(primitive.material)==TSDrawPrimitiveType::TriangleList)
        return ((count+2)/3)*3;
    // The vector-backed convenience layer checks that the exact raw-pointer
    // converter reads fit its owning vector. The address-level C entries below
    // retain the saved unchecked pointer behavior.
    const int elementCount=static_cast<int>(primitive.count);
    if (elementCount<shortStripThreshold+2 && elementCount<3) return 0;
    return count;
}
}
void DTSUnwindStripVector::push_back(std::uint16_t value) {
    ++count;
    void* storage=data;
    if (capacity<count)
        DTS_Vector_resizeStorage(&capacity,&count,&storage,count,2);
    data=static_cast<std::uint16_t*>(storage);
    data[count-1]=value;
}
void unwindTriangleStrip(const std::uint16_t* indices,int count,DTSUnwindStripVector& output) {
    unwindStripCore(indices,count,[&](std::uint16_t index) { output.push_back(index); });
}
std::uint32_t TSMesh::hasDynamicMaterials(TSMaterialList* materials) const {
    std::uint32_t result=static_cast<std::uint32_t>(primitives.size());
    for (const auto& primitive:primitives) {
        if (materials->getFlags(static_cast<int>(drawPrimitiveMaterialIndex(primitive.material)))&
            (TSMaterialFlag::DynamicMaterial|TSMaterialFlag::IFLMaterial))
            return 1;
        result=static_cast<std::uint32_t>(primitives.size());
    }
    return result&0xffffff00u;
}
std::uint32_t TSDecalMesh::hasDynamicMaterials(TSMaterialList* materials) const {
    std::uint32_t result=static_cast<std::uint32_t>(primitives.size());
    for (const auto& primitive:primitives) {
        if (materials->getFlags(static_cast<int>(drawPrimitiveMaterialIndex(primitive.material)))&
            (TSMaterialFlag::DynamicMaterial|TSMaterialFlag::IFLMaterial))
            return 1;
        result=static_cast<std::uint32_t>(primitives.size());
    }
    return result&0xffffff00u;
}
void TSSkinMesh::skinVerticesAndNormals() {
    std::int32_t previousVertex=-1;
    for (std::size_t influence=0; influence<vertexIndices.size(); ++influence) {
        const std::int32_t vertex=vertexIndices[influence];
        const DTSMatrix4& transform=boneTransforms[static_cast<std::size_t>(boneIndices[influence])];
        const TSMeshPoint& sourceVertex=initialVertices[static_cast<std::size_t>(vertex)];
        const TSMeshPoint& sourceNormal=encodedNormals.empty()
            ? initialNormals[static_cast<std::size_t>(vertex)]
            : decodedNormalScratch[static_cast<std::size_t>(vertex)];
        const float weight=weights[influence];
        TSMeshPoint& outputVertex=skinnedVertices[static_cast<std::size_t>(vertex)];
        TSMeshPoint& outputNormal=skinnedNormals[static_cast<std::size_t>(vertex)];
        if (vertex!=previousVertex) {
            outputVertex={0.0f,0.0f,0.0f};
            outputNormal={0.0f,0.0f,0.0f};
            previousVertex=vertex;
        }
        const float* m=transform.values;
        // Preserve the saved expression ordering. In particular, the x87
        // accumulator receives y, z, then x terms for the vector transform.
        outputVertex.x=weight*(sourceVertex.y*m[1]+sourceVertex.z*m[2]+sourceVertex.x*m[0]+m[3])+outputVertex.x;
        outputVertex.y=weight*(sourceVertex.y*m[5]+sourceVertex.z*m[6]+sourceVertex.x*m[4]+m[7])+outputVertex.y;
        outputVertex.z=(sourceVertex.y*m[9]+sourceVertex.z*m[10]+sourceVertex.x*m[8]+m[11])*weight+outputVertex.z;
        outputNormal.x=weight*(sourceNormal.y*m[1]+sourceNormal.z*m[2]+sourceNormal.x*m[0])+outputNormal.x;
        outputNormal.y=weight*(sourceNormal.y*m[5]+sourceNormal.z*m[6]+sourceNormal.x*m[4])+outputNormal.y;
        outputNormal.z=weight*(sourceNormal.y*m[9]+sourceNormal.z*m[10]+sourceNormal.x*m[8])+outputNormal.z;
    }
}
void TSSkinMesh::skinVerticesOnly() {
    std::int32_t previousVertex=-1;
    for (std::size_t influence=0; influence<vertexIndices.size(); ++influence) {
        const std::int32_t vertex=vertexIndices[influence];
        const DTSMatrix4& transform=boneTransforms[static_cast<std::size_t>(boneIndices[influence])];
        const TSMeshPoint& source=initialVertices[static_cast<std::size_t>(vertex)];
        TSMeshPoint& output=skinnedVertices[static_cast<std::size_t>(vertex)];
        const float weight=weights[influence];
        if (vertex!=previousVertex) {
            output={0.0f,0.0f,0.0f};
            previousVertex=vertex;
        }
        const float* m=transform.values;
        output.x=weight*(source.x*m[0]+source.y*m[1]+source.z*m[2]+m[3])+output.x;
        output.y=weight*(source.x*m[4]+source.y*m[5]+source.z*m[6]+m[7])+output.y;
        output.z=weight*(source.x*m[8]+source.y*m[9]+source.z*m[10]+m[11])+output.z;
    }
}
void TSSkinMesh::updateSkin() {
    boneTransforms.resize(nodeIndices.size());
    skinnedVertices.resize(initialVertices.size());
    skinnedNormals.resize(encodedNormals.empty() ? initialNormals.size() : initialVertices.size());
    activeVertices=skinnedVertices.data();
    activeVertexCount=static_cast<int>(skinnedVertices.size());
    activeNormals=skinnedNormals.data();
    activeNormalCount=static_cast<int>(skinnedNormals.size());
    hasActiveStreams=true;
    for (std::size_t bone=0; bone<nodeIndices.size(); ++bone) {
        const float* left=dtsCurrentNodeTransforms[nodeIndices[bone]].values;
        const float* right=initialTransforms[bone].values;
        multiplyMatrix4x4(left,right,boneTransforms[bone].values);
    }
    // 004c2820 tests GL state 0x67 and DAT_0054ec1c. The latter is the
    // environment-map mode set by TSShapeInstance::setupTexturing.
    if (!dtsRenderGL->isEnabled(0x67) && dtsEnvironmentMode==0) {
        skinVerticesOnly();
        return;
    }
    if (!encodedNormals.empty()) {
        // The binary redirects initialNorms to the shared decode scratch
        // buffer. Encoded shapes do not own an initialNormals array, so never
        // write decoded values into that empty normalized vector.
        decodedNormalScratch.resize(initialVertices.size());
        for (int vertex=0; vertex<verticesPerFrame; ++vertex)
            decodedNormalScratch[static_cast<std::size_t>(vertex)]=
                normalDecodeTable[encodedNormals[static_cast<std::size_t>(vertex)]];
    }
    skinVerticesAndNormals();
    for (auto& normal:skinnedNormals) {
        // 004d9d50 receives the squared length as float but returns FSQRT in
        // ST(0), without a float-result store. Preserve that extended result
        // through the threshold, reciprocal and component products.
        const float squaredLength=normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;
        long double scale=std::sqrt(static_cast<long double>(squaredLength));
        if (scale>static_cast<long double>(0.01)) scale=1.0L/scale;
        normal.x=static_cast<float>(scale*static_cast<long double>(normal.x));
        normal.y=static_cast<float>(scale*static_cast<long double>(normal.y));
        normal.z=static_cast<float>(scale*static_cast<long double>(normal.z));
    }
}
void convertToTris(const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
                   const std::uint16_t* source,int count,int* primitiveCount,int* indexCount,
                   TSDrawPrimitive* out,std::uint16_t* output) {
    *primitiveCount=0; *indexCount=0;
    std::uint32_t lastMaterial=0xfffe7961u;
    TSDrawPrimitive* current=nullptr;
    for (int p=0;p<count;++p) {
        std::uint32_t flags=materials[p];
        if (hasDrawPrimitiveFlag(flags,TSDrawPrimitiveFlag::DynamicMaterial)) continue;
        std::uint32_t material=flags&drawPrimitiveMaterialPayloadMask;
        if (material!=lastMaterial) {
            if (out) { current=out+*primitiveCount; current->start=static_cast<std::uint16_t>(*indexCount); current->count=0; current->material=material; }
            ++*primitiveCount; lastMaterial=material;
        }
        auto emit=[&](std::uint16_t a,std::uint16_t b,std::uint16_t c) {
            if (output) { output[*indexCount]=a; output[*indexCount+1]=b; output[*indexCount+2]=c; }
            if (current) current->count=static_cast<std::uint16_t>(current->count+3);
            *indexCount+=3;
        };
        unsigned start=ranges[p][0],n=ranges[p][1];
        if (drawPrimitiveType(flags)==TSDrawPrimitiveType::TriangleList) {
            emitTriangleList(source,start,ranges[p][1],[&](std::uint16_t a,std::uint16_t b,std::uint16_t c) {
                if (output) emit(a,b,c); else emit(0,0,0);
            });
        } else if (n>2) {
            // The original enters the strip loop only after checking count>2,
            // so strips with fewer than three indices perform no source reads.
            std::uint16_t a=source[start],b=source[start+1];
            for (unsigned i=2;i<n;++i) {
                std::uint16_t c=source[start+i];
                if (i&1u) { if (a!=b && b!=c && c!=a) emit(b,a,c); }
                else if (a!=b && b!=c && c!=a) emit(a,b,c);
                a=b; b=c;
            }
        }
    }
}
bool TSMesh::convertToTriangles() {
    struct Range { std::uint16_t values[2]; };
    std::vector<Range> ranges; std::vector<std::uint32_t> materials;
    ranges.reserve(primitives.size()); materials.reserve(primitives.size());
    for (const auto& primitive:primitives) {
        if (!hasDrawPrimitiveFlag(primitive.material,TSDrawPrimitiveFlag::DynamicMaterial)) {
            const std::size_t needed=requiredPrimitiveInput(primitive,0);
            if (primitive.start>indices.size() || needed>indices.size()-primitive.start) return false;
        }
        ranges.push_back({{primitive.start,primitive.count}}); materials.push_back(primitive.material);
    }
    int primitiveCount=0,indexCount=0;
    auto sourceRanges=reinterpret_cast<const std::uint16_t (*)[2]>(ranges.data());
    convertToTris(sourceRanges,materials.data(),indices.data(),static_cast<int>(ranges.size()),&primitiveCount,&indexCount,nullptr,nullptr);
    std::vector<TSDrawPrimitive> resultPrimitives(primitiveCount);
    std::vector<std::uint16_t> resultIndices(indexCount);
    convertToTris(sourceRanges,materials.data(),indices.data(),static_cast<int>(ranges.size()),&primitiveCount,&indexCount,resultPrimitives.data(),resultIndices.data());
    primitives.swap(resultPrimitives); indices.swap(resultIndices); return true;
}
// The saved decompilation only reads DAT_005234dc; no code writes it, so its
// statically zero-initialized value is the strip threshold used here.
int minimumStripTriangles=0;
// Initial values recovered from 005234e4 and 0054ec04 respectively.
int dtsMaximumSkinInfluences=10;
float dtsMinimumSkinInfluenceWeight=0.0f;
// 004c5c90 consults these globals when parsing every mesh. The binary's
// triangle conversion flag and encoded-normal flag are separate from the
// GL primitive mode that selects single-strip versus multiple-strip output.
bool dtsConvertTriangles=false;
bool dtsUseEncodedNormals=false;
static void convertMultipleStrips(const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* source,int count,int* primitiveCount,int* indexCount,
        TSDrawPrimitive* out,std::uint16_t* output,int minimum) {
    *primitiveCount=0; *indexCount=0;
    DTSUnwindStripVector pending;
    std::uint32_t previous=0xfffe7961u;
    auto flush=[&]() {
        if (!pending.count) return;
        // In the saved routine this deferred primitive is finalized only
        // when the index output buffer exists; count-only and split-output
        // calls advance counts without touching its record fields.
        if (out && output) {
            auto& p=out[*primitiveCount]; p.material=clearTriangleStripTypeBit(previous);
            p.start=static_cast<std::uint16_t>(*indexCount); p.count=static_cast<std::uint16_t>(pending.count);
            std::memcpy(output+*indexCount,pending.data,static_cast<std::size_t>(pending.count)*sizeof(std::uint16_t));
        }
        ++*primitiveCount; *indexCount+=static_cast<int>(pending.count); pending.count=0;
    };
    for (int i=0;i<count;++i) {
        std::uint32_t flags=materials[i]; unsigned start=ranges[i][0],n=ranges[i][1];
        if (flags!=previous) flush();
        bool triangles=drawPrimitiveType(flags)==TSDrawPrimitiveType::TriangleList;
        const int elementCount=static_cast<int>(ranges[i][1]);
        if (triangles || elementCount>=minimum+2) {
            TSDrawPrimitive* current=nullptr;
            if (out) { current=out+*primitiveCount; current->start=static_cast<std::uint16_t>(*indexCount); current->count=0; current->material=flags; }
            ++*primitiveCount;
            if (triangles) {
                for (unsigned j=0;j<n;j+=3) {
                    if (output) { output[*indexCount]=source[start+j]; output[*indexCount+1]=source[start+j+1]; output[*indexCount+2]=source[start+j+2]; }
                    if (current) current->count=static_cast<std::uint16_t>(current->count+3);
                    *indexCount+=3;
                }
            } else {
                if (output && n) std::memcpy(output+*indexCount,source+start,n*sizeof(std::uint16_t));
                if (current) current->count=static_cast<std::uint16_t>(n);
                *indexCount+=static_cast<int>(n);
            }
        } else {
                unwindStripCore(source+start,elementCount,
                            [&](std::uint16_t index) { pending.push_back(index); });
        }
        previous=flags;
    }
    flush();
    std::free(pending.data);
}
void leaveAsMultipleStrips(const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* source,int count,int* primitiveCount,int* indexCount,
        TSDrawPrimitive* out,std::uint16_t* output) {
    convertMultipleStrips(ranges,materials,source,count,primitiveCount,indexCount,out,output,minimumStripTriangles);
}
bool TSMesh::leaveAsMultipleStrips(int minimum) {
    struct Range { std::uint16_t values[2]; };
    std::vector<Range> ranges; std::vector<std::uint32_t> materials;
    for (const auto& primitive:primitives) {
        const std::size_t needed=requiredPrimitiveInput(primitive,minimum);
        if (primitive.start>indices.size() || needed>indices.size()-primitive.start) return false;
        ranges.push_back({{primitive.start,primitive.count}}); materials.push_back(primitive.material);
    }
    auto input=reinterpret_cast<const std::uint16_t (*)[2]>(ranges.data());
    int pc=0,ic=0;
    convertMultipleStrips(input,materials.data(),indices.data(),static_cast<int>(ranges.size()),&pc,&ic,nullptr,nullptr,minimum);
    std::vector<TSDrawPrimitive> out(pc); std::vector<std::uint16_t> data(ic);
    convertMultipleStrips(input,materials.data(),indices.data(),static_cast<int>(ranges.size()),&pc,&ic,out.data(),data.data(),minimum);
    primitives.swap(out); indices.swap(data); return true;
}
static void convertSingleStrip(const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* source,int count,int* primitiveCount,std::uint32_t* indexCount,
        TSDrawPrimitive* out,std::uint16_t* output,int minimum) {
    *primitiveCount=0; *indexCount=0;
    DTSUnwindStripVector pending;
    std::uint32_t previous=0xfffe7961u,stripLength=0;
    TSDrawPrimitive* primary=nullptr; TSDrawPrimitive* shortPrimitive=nullptr;
    auto flush=[&]() {
        if (!pending.count) return;
        // The executable writes the short-strip record only alongside its
        // index copy. With no index buffer its metadata is left untouched.
        if (shortPrimitive && out && output) {
            shortPrimitive->start=static_cast<std::uint16_t>(*indexCount);
            shortPrimitive->count=static_cast<std::uint16_t>(pending.count);
            std::memcpy(output+*indexCount,pending.data,static_cast<std::size_t>(pending.count)*sizeof(std::uint16_t));
        }
        *indexCount+=pending.count; pending.count=0; shortPrimitive=nullptr;
    };
    for (int p=0;p<count;++p) {
        std::uint32_t flags=materials[p]; unsigned start=ranges[p][0];
        const int elementCount=static_cast<int>(ranges[p][1]);
        const unsigned n=static_cast<unsigned>(elementCount);
        if (flags!=previous) {
            flush();
            if (out) { primary=out+*primitiveCount; primary->start=static_cast<std::uint16_t>(*indexCount); primary->count=0; primary->material=flags; }
            ++*primitiveCount; previous=flags; stripLength=0;
        }
        if (drawPrimitiveType(flags)==TSDrawPrimitiveType::TriangleList) {
            emitTriangleList(source,start,ranges[p][1],[&](std::uint16_t a,std::uint16_t b,std::uint16_t c) {
            if (output) { output[*indexCount]=a; output[*indexCount+1]=b; output[*indexCount+2]=c; }
                if (primary) primary->count=static_cast<std::uint16_t>(primary->count+3);
                *indexCount+=3;
            });
            continue;
        }
        if (elementCount<minimum+2) {
            if (!pending.count) {
                if (out) { shortPrimitive=out+*primitiveCount; shortPrimitive->material=clearTriangleStripTypeBit(flags); }
                ++*primitiveCount;
            }
            unwindStripCore(source+start,elementCount,
                            [&](std::uint16_t index) { pending.push_back(index); });
            // The original reserves these main-strip slots without writing them.
        } else if (output) {
            std::uint32_t extra=stripLength ? ((stripLength&1u)|2u) : 0;
            if (extra) {
                output[*indexCount]=output[*indexCount-1];
                if (extra==3) output[*indexCount+1]=output[*indexCount-1];
                output[*indexCount+extra-1]=source[start];
            }
            if (n) std::memcpy(output+*indexCount+extra,source+start,n*sizeof(std::uint16_t));
        }
        std::uint32_t added=n+(stripLength ? ((stripLength&1u)|2u) : 0);
        if (primary) primary->count=static_cast<std::uint16_t>(primary->count+added);
        stripLength+=added; *indexCount+=added;
    }
    flush();
    std::free(pending.data);
}
void convertToSingleStrip(const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* source,int count,int* primitiveCount,std::uint32_t* indexCount,
        TSDrawPrimitive* out,std::uint16_t* output) {
    convertSingleStrip(ranges,materials,source,count,primitiveCount,indexCount,out,output,minimumStripTriangles);
}
bool TSMesh::convertToSingleStrip(int minimum) {
    struct Range { std::uint16_t values[2]; };
    std::vector<Range> ranges; std::vector<std::uint32_t> materials;
    for (const auto& p:primitives) {
        const std::size_t needed=requiredPrimitiveInput(p,minimum);
        if (p.start>indices.size() || needed>indices.size()-p.start) return false;
        ranges.push_back({{p.start,p.count}}); materials.push_back(p.material);
    }
    auto input=reinterpret_cast<const std::uint16_t (*)[2]>(ranges.data());
    int pc=0; std::uint32_t ic=0;
    convertSingleStrip(input,materials.data(),indices.data(),static_cast<int>(ranges.size()),&pc,&ic,nullptr,nullptr,minimum);
    std::vector<TSDrawPrimitive> out(pc); std::vector<std::uint16_t> data(ic);
    convertSingleStrip(input,materials.data(),indices.data(),static_cast<int>(ranges.size()),&pc,&ic,out.data(),data.data(),minimum);
    primitives.swap(out); indices.swap(data); return true;
}
bool TSMesh::assembleBase(TSShapeAlloc& a,std::uint32_t version,bool encoded,const std::vector<TSMesh>& previous) {
    // Assembly replaces the base geometry pointers. A previously skinned
    // normalized object must stop exposing its old redirected buffers here.
    hasActiveStreams=false;
    activeVertices=nullptr; activeNormals=nullptr;
    activeVertexCount=activeNormalCount=0;
    auto real=[&]() { std::uint32_t word=a.get32(); float value; std::memcpy(&value,&word,4); return value; };
    auto count=[&](std::uint32_t& value) { value=a.get32(); return true; };
    auto copyWords=[&](void* output,int words) {
        const auto* source=a.buffer32;
        if (a.multiplier) a.copyToShape32(words,0); else a.getPointer32(words);
        if (words) std::memcpy(output,source,static_cast<std::size_t>(words)*4);
    };
    auto copyBytes=[&](void* output,int bytes) {
        const auto* source=a.buffer8;
        if (a.multiplier) a.copyToShape8(bytes,0); else a.getPointer8(bytes);
        if (bytes) std::memcpy(output,source,static_cast<std::size_t>(bytes));
    };
    auto copySharedWords=[&](const void* source,int words) {
        if (a.multiplier && activeMeshAssembly && activeMeshAssembly->retainedMeshes &&
            !activeMeshAssembly->retainedMeshes[parentMesh]) {
            void* destination=a.allocShape32(words);
            if (destination && source) std::memcpy(destination,source,static_cast<std::size_t>(words)*4);
        }
    };
    auto copySharedBytes=[&](const void* source,int bytes) {
        if (a.multiplier && activeMeshAssembly && activeMeshAssembly->retainedMeshes &&
            !activeMeshAssembly->retainedMeshes[parentMesh]) {
            void* destination=a.allocShape8(bytes);
            // Preserve the saved helper's count-byte allocation and count*4
            // copy for parent encoded normals.
            if (destination && source) std::memcpy(destination,source,static_cast<std::size_t>(bytes)*4);
        }
    };
    a.checkGuard(); numFrames=a.get32(); numMaterialFrames=a.get32(); parentMesh=static_cast<std::int32_t>(a.get32());
    for (auto& value:bounds) value=real(); for (auto& value:center) value=real();
    // 004c5cfd uses FILD on the saved word, followed by FSTP to mRadius.
    radius=static_cast<float>(static_cast<std::int32_t>(a.get32()));
    std::uint32_t vertexCount=0,texcoordCount=0;
    if (!count(vertexCount)) return false;
    if (parentMesh>=0) {
        if (static_cast<std::size_t>(parentMesh)>=previous.size()) return false;
        vertices=previous[parentMesh].vertices;
        if (vertices.size()<vertexCount) return false;
        vertices.resize(vertexCount);
        copySharedWords(previous[parentMesh].vertices.data(),static_cast<int>(vertexCount)*3);
    } else {
        vertices.resize(vertexCount); copyWords(vertices.data(),static_cast<int>(vertexCount)*3);
    }
    if (!count(texcoordCount)) return false;
    if (parentMesh>=0) {
        texcoords=previous[parentMesh].texcoords;
        if (texcoords.size()<texcoordCount) return false;
        texcoords.resize(texcoordCount);
        copySharedWords(previous[parentMesh].texcoords.data(),static_cast<int>(texcoordCount)*2);
    } else {
        texcoords.resize(texcoordCount); copyWords(texcoords.data(),static_cast<int>(texcoordCount)*2);
    }
    normals.clear(); encodedNormals.clear();
    if (version<22 || !encoded) {
        if (parentMesh>=0) { normals=previous[parentMesh].normals; if (normals.size()<vertexCount) return false; normals.resize(vertexCount); copySharedWords(previous[parentMesh].normals.data(),static_cast<int>(vertexCount)*3); }
        else { normals.resize(vertexCount); copyWords(normals.data(),static_cast<int>(vertexCount)*3); }
        if (version>=22 && parentMesh<0) a.getPointer8(vertexCount);
    } else {
        if (parentMesh>=0) { encodedNormals=previous[parentMesh].encodedNormals; if (encodedNormals.size()<vertexCount) return false; encodedNormals.resize(vertexCount); }
        else {
            a.getPointer32(vertexCount*3); encodedNormals.resize(vertexCount);
            copyBytes(encodedNormals.data(),static_cast<int>(vertexCount));
        }
        if (parentMesh>=0) copySharedBytes(previous[parentMesh].encodedNormals.data(),static_cast<int>(vertexCount));
    }
    std::uint32_t primitiveCount=0,indexCount=0,mergeCount=0;
    if (!count(primitiveCount)) return false;
    primitives.resize(primitiveCount);
    for (auto& primitive:primitives) { primitive.start=a.get16(); primitive.count=a.get16(); }
    for (auto& primitive:primitives) primitive.material=a.get32();
    if (!count(indexCount)) return false;
    indices.resize(indexCount); for (auto& index:indices) index=a.get16();
    if (activeMeshAssembly && !convertAssemblyPrimitives(*this,*activeMeshAssembly,(meshType&7)==3)) return false;
    // 004c5c90 sizes converted primitive and index output before merge data.
    auto* primitiveOutput=a.allocShape32(static_cast<int>(primitives.size())*2);
    if (primitiveOutput && a.multiplier && !primitives.empty())
        std::memcpy(primitiveOutput,primitives.data(),primitives.size()*sizeof(TSDrawPrimitive));
    a.align32();
    auto* indexOutput=a.allocShape16(static_cast<int>(indices.size()));
    if (indexOutput && a.multiplier && !indices.empty())
        std::memcpy(indexOutput,indices.data(),indices.size()*sizeof(std::uint16_t));
    a.align32();
    if (!count(mergeCount)) return false;
    mergeIndices.resize(mergeCount);
    const auto* mergeSource=a.buffer16;
    a.copyToShape16(static_cast<int>(mergeCount),0);
    if (mergeCount) std::memcpy(mergeIndices.data(),mergeSource,mergeCount*2);
    a.align32();
    verticesPerFrame=a.get32(); std::uint32_t flags=a.get32();
    flags&=~0x08000000u;
    if (numFrames==1 && numMaterialFrames==1 && !(flags&0xa0000000u) && (meshType&7)!=3) flags|=0x08000000u;
    meshType|=flags; a.checkGuard();
    if (version<19 && a.destination) computeBounds();
    return true;
}
bool TSSortedMesh::assemble(TSShapeAlloc& a,std::uint32_t version,bool encoded,const std::vector<TSMesh>& previous) {
    meshType=3;
    if (!assembleBase(a,version,encoded,previous)) return false;
    auto count=[&](std::uint32_t& n) { n=a.get32(); return true; };
    std::uint32_t n=0;
    if (!count(n)) return false; clusters.resize(n);
    const auto* clusterSource=a.buffer32;
    a.copyToShape32(static_cast<int>(n*8),0);
    if (n) std::memcpy(clusters.data(),clusterSource,n*8*4);
    auto readArray=[&](std::vector<std::int32_t>& values) {
        std::uint32_t size=0; if (!count(size)) return false;
        values.resize(size); const auto* source=a.buffer32;
        a.copyToShape32(static_cast<int>(size),0);
        if (size) std::memcpy(values.data(),source,size*4); return true;
    };
    if (!readArray(startCluster) || !readArray(firstVertices) || !readArray(numVertices) || !readArray(firstTexcoords)) return false;
    alwaysWriteDepth=a.get32()!=0; a.checkGuard(); return true;
}
bool TSSkinMesh::assemble(TSShapeAlloc& a,std::uint32_t version,bool encoded,
                          const std::vector<TSMesh>& previous,const std::vector<TSSkinMesh>& previousSkins) {
    meshType=1;
    initialVertices.clear();
    if (!assembleBase(a,version,encoded,previous)) return false;
    auto count=[&](std::uint32_t& n) { n=a.get32(); return true; };
    // The original shared-data helpers reserve another copy only when the
    // parent was skipped and this mesh is retained. Normalized vectors still
    // own their copies; these writes account for the original arena payload.
    auto copyWords=[&](void* output,int words,const void* shared) {
        if (shared) {
            if (a.multiplier && activeMeshAssembly && activeMeshAssembly->retainedMeshes &&
                !activeMeshAssembly->retainedMeshes[parentMesh]) {
                void* dest=a.allocShape32(words);
                if (dest) std::memcpy(dest,shared,static_cast<std::size_t>(words)*4);
            }
        } else {
            const auto* source=a.buffer32;
            if (a.multiplier) a.copyToShape32(words,0); else a.getPointer32(words);
            if (words) std::memcpy(output,source,static_cast<std::size_t>(words)*4);
        }
    };
    auto copySharedBytes=[&](const void* source,int bytes) {
        if (a.multiplier && activeMeshAssembly && activeMeshAssembly->retainedMeshes &&
            !activeMeshAssembly->retainedMeshes[parentMesh]) {
            void* destination=a.allocShape8(bytes);
            // Match TSMesh_getSharedData8_impl: reserve count bytes and copy
            // count*4 bytes when an unretained parent stream is duplicated.
            if (destination && source) std::memcpy(destination,source,static_cast<std::size_t>(bytes)*4);
        }
    };
    std::uint32_t n=0;
    if (!count(n)) return false;
    const std::uint32_t initialVertexCount=n;
    const TSSkinMesh* parent=nullptr;
    if (parentMesh>=0) { if (static_cast<std::size_t>(parentMesh)>=previousSkins.size()) return false; parent=&previousSkins[parentMesh]; }
    // Initial vertices and normals use the common mesh scratch tables. For
    // the separate skin list those tables contain initial data; for the main
    // mesh list they contain the parent's base data.
    const TSMesh* sharedBase=parent ? &previous[parentMesh] : nullptr;
    if (parent) { initialVertices=sharedBase->vertices; if (initialVertices.size()<n) return false; initialVertices.resize(n); copyWords(nullptr,static_cast<int>(n*3),sharedBase->vertices.data()); }
    else { initialVertices.resize(n); copyWords(initialVertices.data(),static_cast<int>(n*3),nullptr); }
    if (version<22 || !encoded) {
        if (parent) { initialNormals=sharedBase->normals; if (initialNormals.size()<n) return false; initialNormals.resize(n); copyWords(nullptr,static_cast<int>(n*3),sharedBase->normals.data()); }
        else { initialNormals.resize(n); copyWords(initialNormals.data(),static_cast<int>(n*3),nullptr); }
        if (version>=22 && !parent) a.getPointer8(n);
        encodedNormals.clear();
    } else {
        initialNormals.clear();
        if (parent) {
            encodedNormals=sharedBase->encodedNormals;
            if (encodedNormals.size()<n) return false;
            encodedNormals.resize(n);
            copySharedBytes(sharedBase->encodedNormals.data(),static_cast<int>(n));
        }
        else {
            a.getPointer32(n*3); encodedNormals.resize(n);
            const auto* source=a.buffer8;
            if (a.multiplier) a.copyToShape8(static_cast<int>(n),0); else a.getPointer8(n);
            if (n) std::memcpy(encodedNormals.data(),source,n);
        }
    }
    if (!count(n)) return false;
    if (parent) { initialTransforms=parent->initialTransforms; if (initialTransforms.size()<n) return false; initialTransforms.resize(n); copyWords(nullptr,static_cast<int>(n*16),parent->initialTransforms.data()); }
    else { initialTransforms.resize(n); copyWords(initialTransforms.data(),static_cast<int>(n*16),nullptr); }
    if (!count(n)) return false;
    std::vector<const std::int32_t*> parentVertexData(previousSkins.size());
    std::vector<const std::int32_t*> parentBoneData(previousSkins.size());
    std::vector<const float*> parentWeightData(previousSkins.size());
    for (std::size_t i=0;i<previousSkins.size();++i) {
        parentVertexData[i]=previousSkins[i].vertexIndices.data();
        parentBoneData[i]=previousSkins[i].boneIndices.data();
        parentWeightData[i]=previousSkins[i].weights.data();
    }
    int influenceCount=static_cast<int>(n);
    const int sourceInfluenceCount=influenceCount;
    const std::int32_t* sourceVertices=nullptr;
    const std::int32_t* sourceBones=nullptr;
    const float* sourceWeights=nullptr;
    if (sourceInfluenceCount>0 && parentMesh<0) {
        sourceVertices=reinterpret_cast<const std::int32_t*>(a.buffer32);
        sourceBones=sourceVertices+sourceInfluenceCount;
        sourceWeights=reinterpret_cast<const float*>(sourceBones+sourceInfluenceCount);
    } else if (sourceInfluenceCount>0) {
        sourceVertices=parentVertexData[static_cast<std::size_t>(parentMesh)];
        sourceBones=parentBoneData[static_cast<std::size_t>(parentMesh)];
        sourceWeights=parentWeightData[static_cast<std::size_t>(parentMesh)];
    }
    std::int32_t* sharedVertices=nullptr;
    std::int32_t* sharedBones=nullptr;
    float* sharedWeights=nullptr;
    getSharedSkinInfluences(static_cast<int>(initialVertexCount),parentMesh,
        &influenceCount,parentVertexData.data(),parentBoneData.data(),parentWeightData.data(),
        &sharedVertices,&sharedBones,&sharedWeights,activeMeshAssembly->skip ? 1 : 0);
    n=static_cast<std::uint32_t>(influenceCount);
    vertexIndices.resize(n); boneIndices.resize(n); weights.resize(n);
    if (n) {
        if (sharedVertices && sharedBones && sharedWeights) {
            std::memcpy(vertexIndices.data(),sharedVertices,static_cast<std::size_t>(n)*sizeof(*sharedVertices));
            std::memcpy(boneIndices.data(),sharedBones,static_cast<std::size_t>(n)*sizeof(*sharedBones));
            std::memcpy(weights.data(),sharedWeights,static_cast<std::size_t>(n)*sizeof(*sharedWeights));
        } else {
            // During the allocator sizing pass the recovered shared-influence
            // helper has counted and reserved the reduced arena ranges but
            // returned null output slots. The normalized vectors still need
            // the same reduced values for the final pass and mesh references.
            const int written=reduceSkinInfluences(sourceVertices,sourceBones,sourceWeights,
                vertexIndices.data(),boneIndices.data(),weights.data(),sourceInfluenceCount);
            if (written!=static_cast<int>(n)) return false;
        }
    }
    if (!count(n)) return false;
    if (parent) { nodeIndices=parent->nodeIndices; if (nodeIndices.size()<n) return false; nodeIndices.resize(n); copyWords(nullptr,static_cast<int>(n),parent->nodeIndices.data()); }
    else { nodeIndices.resize(n); copyWords(nodeIndices.data(),static_cast<int>(n),nullptr); }
    a.checkGuard(); if (version<19 && a.destination) TSMesh::computeBounds(); return true;
}
bool TSSkinMesh::reduceInfluences(const DTSSkinInfluencePolicy& policy) {
    if (vertexIndices.size()!=boneIndices.size() || vertexIndices.size()!=weights.size()) return false;
    std::vector<std::int32_t> vertices,bones;
    std::vector<float> reducedWeights;
    vertices.reserve(vertexIndices.size()); bones.reserve(boneIndices.size()); reducedWeights.reserve(weights.size());
    std::size_t i=0;
    while (i<vertexIndices.size()) {
        std::size_t keep=1;
        while (i+keep<vertexIndices.size() && vertexIndices[i+keep]==vertexIndices[i] &&
               static_cast<int>(keep)<policy.maxPerVertex && !(weights[i+keep]<policy.minimumWeight)) ++keep;
        float sum=0;
        for (std::size_t j=0;j<keep;++j) sum+=weights[i+j];
        const long double inverse=1.0L/static_cast<long double>(sum);
        for (std::size_t j=0;j<keep;++j) {
            vertices.push_back(vertexIndices[i+j]); bones.push_back(boneIndices[i+j]); reducedWeights.push_back(static_cast<float>(inverse*weights[i+j]));
        }
        std::size_t next=i+keep;
        while (next<vertexIndices.size() && vertexIndices[next]==vertexIndices[i]) ++next;
        i=next;
    }
    vertexIndices.swap(vertices); boneIndices.swap(bones); weights.swap(reducedWeights); return true;
}
bool TSDecalMesh::assemble(TSShapeAlloc& a,std::uint32_t version) {
    auto count=[&](std::uint32_t& n) { n=a.get32(); return true; };
    if (version<20) { a.checkGuard(); a.getPointer32(15); }
    std::uint32_t n=0;
    if (!count(n)) return false; primitives.resize(n);
    for (auto& primitive:primitives) {
        std::memcpy(&primitive.start,a.buffer16,4); a.copyToShape16(2,0);
        std::memcpy(&primitive.material,a.buffer32,4); a.copyToShape32(1,0);
    }
    a.align32();
    if (!count(n)) return false; indices.resize(n);
    const auto* indexSource=a.buffer16; a.copyToShape16(static_cast<int>(n),0);
    if (n) std::memcpy(indices.data(),indexSource,n*2);
    a.align32();
    if (version<20) { a.getPointer32(3); a.checkGuard(); }
    if (!count(n)) return false; startPrimitive.resize(n);
    const auto* startSource=a.buffer32; a.copyToShape32(static_cast<int>(n),0);
    if (n) std::memcpy(startPrimitive.data(),startSource,n*4);
    texgenS.clear(); texgenT.clear();
    if (version>=19) {
        texgenS.resize(n); texgenT.resize(n);
        const auto* sourceS=a.buffer32; a.copyToShape32(static_cast<int>(n*4),0);
        if (n) std::memcpy(texgenS.data(),sourceS,n*16);
        const auto* sourceT=a.buffer32; a.copyToShape32(static_cast<int>(n*4),0);
        if (n) std::memcpy(texgenT.data(),sourceT,n*16);
    }
    materialIndex=a.get32(); meshType=startPrimitive.size()==1 ? 0x08000000u : 0;
    a.checkGuard(); return true;
}

int reduceSkinInfluences(const std::int32_t* vertexIndices,const std::int32_t* boneIndices,
        const float* weights,std::int32_t* outputVertices,std::int32_t* outputBones,
        float* outputWeights,int influenceCount) {
    int input=0,output=0;
    while (input<influenceCount) {
        int kept=1;
        while (input+kept<influenceCount && vertexIndices[input+kept]==vertexIndices[input] &&
               dtsMaximumSkinInfluences>kept &&
               !(weights[input+kept]<dtsMinimumSkinInfluenceWeight)) ++kept;
        if (outputVertices) {
            std::memcpy(outputVertices+output,vertexIndices+input,static_cast<std::size_t>(kept)*4);
            std::memcpy(outputBones+output,boneIndices+input,static_cast<std::size_t>(kept)*4);
            std::memcpy(outputWeights+output,weights+input,static_cast<std::size_t>(kept)*4);
            float sum=0.0f;
            for (int i=0;i<kept;++i) sum=sum+outputWeights[output+i];
            const long double inverse=1.0L/static_cast<long double>(sum);
            for (int i=0;i<kept;++i)
                outputWeights[output+i]=static_cast<float>(inverse*outputWeights[output+i]);
        }
        int next=input+kept;
        output+=kept;
        while (next<influenceCount && vertexIndices[next]==vertexIndices[input]) ++next;
        input=next;
    }
    return output;
}

void getSharedSkinInfluences(int initialVertexCount,int parentMesh,int* influenceCount,
        const std::int32_t* const* vertexData,const std::int32_t* const* boneData,
        const float* const* weightData,std::int32_t** outputVertices,
        std::int32_t** outputBones,float** outputWeights,char skip) {
    auto& context=*activeMeshAssembly;
    auto& allocator=*context.allocator;
    if (parentMesh<0) {
        auto* vertices=static_cast<const std::int32_t*>(allocator.getPointer32(*influenceCount));
        auto* bones=static_cast<const std::int32_t*>(allocator.getPointer32(*influenceCount));
        auto* weights=static_cast<const float*>(allocator.getPointer32(*influenceCount));
        if (skip) {
            *outputVertices=const_cast<std::int32_t*>(vertices);
            *outputBones=const_cast<std::int32_t*>(bones);
            *outputWeights=const_cast<float*>(weights);
            return;
        }
        const int reduced=reduceSkinInfluences(vertices,bones,weights,nullptr,nullptr,nullptr,*influenceCount);
        *outputVertices=static_cast<std::int32_t*>(allocator.allocShape32(reduced));
        *outputBones=static_cast<std::int32_t*>(allocator.allocShape32(reduced));
        *outputWeights=static_cast<float*>(allocator.allocShape32(reduced));
        if (*outputVertices)
            reduceSkinInfluences(vertices,bones,weights,*outputVertices,*outputBones,*outputWeights,*influenceCount);
        *influenceCount=reduced;
        return;
    }
    const auto* vertices=vertexData[parentMesh];
    const auto* bones=boneData[parentMesh];
    const auto* weights=weightData[parentMesh];
    if (!context.retainedMeshes[parentMesh] && !skip) {
        const int reduced=reduceSkinInfluences(vertices,bones,weights,nullptr,nullptr,nullptr,*influenceCount);
        *outputVertices=static_cast<std::int32_t*>(allocator.allocShape32(reduced));
        *outputBones=static_cast<std::int32_t*>(allocator.allocShape32(reduced));
        *outputWeights=static_cast<float*>(allocator.allocShape32(reduced));
        if (*outputVertices)
            reduceSkinInfluences(vertices,bones,weights,*outputVertices,*outputBones,*outputWeights,*influenceCount);
        *influenceCount=reduced;
        return;
    }
    *outputVertices=const_cast<std::int32_t*>(vertices);
    *outputBones=const_cast<std::int32_t*>(bones);
    *outputWeights=const_cast<float*>(weights);
    int prefix=0;
    if (vertices) {
        while (prefix<*influenceCount && vertices[prefix]<initialVertexCount) ++prefix;
    }
    *influenceCount=prefix;
}
} }

// Address-level wrappers keep the original assembly entry ABIs explicit;
// the normalized helpers obtain allocator, stream and parent-table state from
// activeMeshAssembly rather than changing these entry signatures.

