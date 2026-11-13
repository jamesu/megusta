// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include "vector_storage.h"
namespace megusta { class ResourceFile; namespace dts {
// Fixed-width set used by the executable's shape sequences.
class TSIntegerSet {
public:
    static constexpr unsigned bitsPerWord=32;
    static constexpr unsigned wordCount=6;
    static constexpr unsigned capacity=bitsPerWord*wordCount;
    std::uint32_t bits[wordCount];
    TSIntegerSet();
    TSIntegerSet(const TSIntegerSet& other);
    void copy(const TSIntegerSet& other);
    bool test(std::uint32_t index) const;
    void set(std::uint32_t index);
    void clear(std::uint32_t index);
    void clearAll(std::uint32_t count);
    void setAll(std::uint32_t count);
    bool testAll(std::uint32_t count) const;
    void overlap(const TSIntegerSet& other);
    void takeAway(const TSIntegerSet& other);
    int start() const;
    int end() const;
    void next(std::uint32_t* index) const;
    void read(ResourceFile* file);
};
static_assert(sizeof(TSIntegerSet)==24,"DTS integer-set width");
} }

