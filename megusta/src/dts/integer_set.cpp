#include "integer_set.h"
#include "../engine/resource_file.h"
#include <cstring>
#include <new>
namespace megusta { namespace dts {
TSIntegerSet::TSIntegerSet() { clearAll(capacity); }
TSIntegerSet::TSIntegerSet(const TSIntegerSet& other) { copy(other); }
void TSIntegerSet::copy(const TSIntegerSet& other) { std::memcpy(bits,other.bits,sizeof(bits)); }
bool TSIntegerSet::test(std::uint32_t index) const {
    if (index>=capacity) return false;
    return (bits[index/bitsPerWord]&(1u<<(index&(bitsPerWord-1))))!=0;
}
void TSIntegerSet::set(std::uint32_t index) {
    if (index>=capacity) return;
    bits[index/bitsPerWord]|=1u<<(index&(bitsPerWord-1));
}
void TSIntegerSet::clear(std::uint32_t index) {
    if (index>=capacity) return;
    bits[index/bitsPerWord]&=~(1u<<(index&(bitsPerWord-1)));
}
void TSIntegerSet::clearAll(std::uint32_t count) {
    int words=static_cast<std::int32_t>(count)/bitsPerWord; std::memset(bits,0,words*sizeof(bits[0]));
    if (count%bitsPerWord) bits[words]&=~(((1u<<(count%bitsPerWord))*2u)-1u);
}
void TSIntegerSet::setAll(std::uint32_t count) {
    int words=static_cast<std::int32_t>(count)/bitsPerWord; std::memset(bits,0xff,words*sizeof(bits[0]));
    if (count%bitsPerWord) bits[words]|=((1u<<(count%bitsPerWord))*2u)-1u;
}
bool TSIntegerSet::testAll(std::uint32_t count) const {
    int words=static_cast<std::int32_t>(count)/bitsPerWord;
    for (int i=0;i<words;++i) if (bits[i]) return true;
    return (count%bitsPerWord) && (bits[words]&(((1u<<(count%bitsPerWord))*2u)-1u));
}
void TSIntegerSet::overlap(const TSIntegerSet& other) { for (unsigned i=0;i<wordCount;++i) bits[i]|=other.bits[i]; }
void TSIntegerSet::takeAway(const TSIntegerSet& other) { for (unsigned i=0;i<wordCount;++i) bits[i]&=~other.bits[i]; }
int TSIntegerSet::start() const {
    for (int i=0;i<capacity;++i) if (bits[i/bitsPerWord]&(1u<<(i&(bitsPerWord-1)))) return i;
    return capacity;
}
int TSIntegerSet::end() const {
    for (int i=static_cast<int>(capacity)-1;i>=0;--i) if (bits[i/bitsPerWord]&(1u<<(i&(bitsPerWord-1)))) return i+1;
    return 0;
}
void TSIntegerSet::next(std::uint32_t* index) const {
    ++*index;
    while (*index<capacity && !test(*index)) ++*index;
}
void TSIntegerSet::read(ResourceFile* file) {
    clearAll(capacity); std::uint32_t ignored=0,count=0;
    file->read(&ignored,4,1); file->read(&count,4,1); file->read(bits,4,count);
}
} }

