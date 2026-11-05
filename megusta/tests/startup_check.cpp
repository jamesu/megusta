#include "game/startup.h"
#include "game/gui_assets.h"
#include "engine/reader_stream.h"
#include <GL/glpng.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
namespace {
struct Observed { int menus[2]{},games[2]{},autoruns=0,delay=0; bool startNow=false; };
}
int checkStartupArguments() {
    Observed observed;
    megusta::StartupServices services{};
    services.context=&observed;
    services.autorun=[](void* context,int menu,int game,bool start) {
        auto& value=*static_cast<Observed*>(context);
        value.menus[value.autoruns]=menu; value.games[value.autoruns++]=game;
        value.startNow=start;
    };
    services.cameraDelay=[](void* context,int delay) { static_cast<Observed*>(context)->delay=delay; };
    char args[][40]={"minigolf","-SCENELOADMETHOD:3","-forcepix","-buildtbl","-demo",
        "-memdebug:-nointro","-autorun:12,34","-autorun:7","-camdelay:-8","-alpha:128"};
    char* argv[10]; for (int i=0;i<10;++i) argv[i]=args[i];
    megusta::StartupState state{};
    megusta::parseStartupArguments(10,argv,state,services);
    if (!state.intro || !state.forcePix || !state.buildTables || !state.demo ||
        state.sceneLoadMethod!=3 || state.alpha!=128 || observed.autoruns!=2 ||
        observed.menus[0]!=12 || observed.games[0]!=34 || observed.menus[1]!=7 ||
        observed.games[1]!=0 || observed.startNow || observed.delay!=-8) return 1;
    char noIntro[]="-NoIntro",unknown[]="-nointro-extra";
    char* second[]={args[0],noIntro,unknown};
    megusta::parseStartupArguments(3,second,state,services);
    if (state.intro) return 1;
    struct AtlasProbe {
        megusta::GuiTextureAtlas atlas;
        int calls=0;
        bool options=true;
        const char* path=nullptr;
    } atlas;
    atlas.atlas.textureCount=9;
    megusta::GuiAssetServices assets{&atlas,[](void* context,const char* path,bool external,int mipmap,int wrap,int min,int mag) {
        auto& p=*static_cast<AtlasProbe*>(context);
        ++p.calls; p.path=path;
        p.options&=external && mipmap==0 && wrap==0x2900 && min==0x2600 && mag==0x2600;
        return &p.atlas;
    }};
    megusta::GuiWidgetResources widgets;
    widgets.loadLozenge(assets);
    if (!widgets.lozengeInitialized || widgets.lozengeAtlas!=&atlas.atlas || atlas.calls!=1 ||
        !atlas.options || std::strcmp(atlas.path,"data/textures/gui/LOZLR.mpx")) return 2;
    widgets.loadShadowAtlas(assets); widgets.loadShadowAtlas(assets);
    widgets.markTextInitialized(); widgets.markSimpleButtonInitialized();
    if (atlas.calls!=2 || !atlas.options || widgets.shadowAtlas!=&atlas.atlas ||
        !widgets.textInitialized || !widgets.simpleButtonInitialized ||
        std::strcmp(atlas.path,"data/textures/gui/SHADBX.mpx")) return 3;
    megusta::ResourceMountTable mounts;
    auto actualAssets=megusta::guiAssetServices(mounts);
    std::unique_ptr<megusta::GuiTextureAtlas> empty(actualAssets.loadAtlas(actualAssets.context,"missing.mpx",true,0,0x2900,0x2600,0x2600));
    if (!empty || empty->textureCount!=0) return 4;
    empty->textureNames.reset(new std::uint32_t[1]{123}); empty->textureCount=1;
    if (megusta::pix::getMultiTexture(empty.get(),0)!=123) return 5;
    std::unique_ptr<megusta::GuiTextureAtlas> noExtension(actualAssets.loadAtlas(actualAssets.context,"missing",false,0,0x2900,0x2600,0x2600));
    if (noExtension) return 6;
    // A complete 1x1 black grayscale PNG, generated from filter byte 0 and
    // pixel byte 0 with zlib and standard PNG chunk CRCs. No filesystem or GL.
    const unsigned char png[]={
        0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0,0,0,0x0d,0x49,0x48,0x44,0x52,
        0,0,0,1,0,0,0,1,8,0,0,0,0,0x3a,0x7e,0x9b,0x55,
        0,0,0,0x0a,0x49,0x44,0x41,0x54,0x78,0x9c,0x63,0x60,0,0,0,2,0,1,
        0x48,0xaf,0xa4,0x71,0,0,0,0,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82};
    megusta::ResourceFile image(png,sizeof(png));
    auto reader=megusta::resourceReader(image);
    unsigned char signature[8];
    if (reader.read(reader.context,signature,2,4)!=4 || std::memcmp(signature,png,8) ||
        reader.seek(reader.context,-8,SEEK_CUR)!=0) return 7;
    pngRawInfo info{};
    if (!pngLoadRawReader(&reader,&info)) return 8;
    bool decoded=info.Width==1 && info.Height==1 && info.Depth==8 && info.Components==1 && info.Data && info.Data[0]==0;
    std::free(info.Data);
    if (!decoded || image.tell()<=8) return 9;
    megusta::ResourceFile shortImage(png,7);
    auto shortReader=megusta::resourceReader(shortImage);
    if (pngLoadRawReader(&shortReader,&info)!=0) return 10;
    megusta::ResourceFile brokenImage(png,32);
    auto brokenReader=megusta::resourceReader(brokenImage);
    if (pngLoadRawReader(&brokenReader,&info)!=0) return 11;
    std::puts("Startup argument checks passed (00402360)");
    std::puts("GUI startup atlas options, shared shadow-cache and widget flags passed (mock loader; no GL)");
    std::puts("Unified resource reader: read/seek, in-memory PNG decode and truncated input passed; no temp files or GL");
    return 0;
}
