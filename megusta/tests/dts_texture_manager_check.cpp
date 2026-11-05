#include "../src/dts/texture_manager.h"
#include "../src/dts/bitmap_support.h"
#include <cassert>

namespace {
int destroyed=0;
void destroyEntry(megusta::dts::EngineTextureEntry* entry) {
    ++destroyed;
    megusta::dts::unlinkTextureEntry(entry);
    delete entry;
}
}

int main() {
    using namespace megusta::dts;
    assert(FUN_004ce4e0(reinterpret_cast<const std::uint8_t*>("Ab"))==3);
    FUN_004cd9f0();
    assert(textureEntryBucketCount==256 && textureEntryList==nullptr);
    auto* first=new EngineTextureEntry;
    auto* second=new EngineTextureEntry;
    auto* third=new EngineTextureEntry;
    first->filename="a";
    second->filename="A";
    third->filename="z";
    FUN_004cdb00(first);
    FUN_004cdb00(second);
    FUN_004cdb00(third);
    assert(textureEntryList==third && third->next==second && second->next==first);
    assert(second->previous==third && first->previous==second);
    const auto collision=textureFilenameHash(reinterpret_cast<const std::uint8_t*>("a"));
    assert(textureEntryBuckets[collision%textureEntryBucketCount]==second);
    FUN_004cda90(second);
    assert(third->next==first && first->previous==third);
    assert(textureEntryBuckets[collision%textureEntryBucketCount]==first);
    delete second;

    EngineBitmapView bitmap{};
    bitmap.width=2;bitmap.height=2;bitmap.mipCount=1;
    auto* cached=new EngineTextureEntry;
    cached->filename="cached";
    cached->kind=1;
    cached->referenceCount=4;
    cached->bitmap=&bitmap;
    FUN_004cdb00(cached);
    assert(createOrReplaceTextureEntry("cached",&bitmap,1,false)==cached);
    assert(cached->referenceCount==4);
    cached->bitmap=nullptr;

    bindTextureEntryDestroy(destroyEntry);
    FUN_004cdb60();
    assert(destroyed==3 && textureEntryList==nullptr && textureEntryBuckets==nullptr);
    assert(textureEntryBucketCount==256);
    bindTextureEntryDestroy(nullptr);
}
