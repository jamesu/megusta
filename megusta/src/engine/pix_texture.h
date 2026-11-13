// SPDX-License-Identifier: GPL-2.0

#pragma once
// Binary-derived PIX interface; original spellings of private fields unknown.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <GL/megusta_reader.h>
namespace megusta::pix {
struct TextureInfo {
    int32_t width,height,bits_per_pixel,alpha,format,mipmap_mode;
};
size_t decode_flagged(const unsigned char*,size_t,unsigned char*,size_t);
size_t decode_runs(const unsigned char*,size_t,unsigned char*,size_t);
bool load(FILE*,TextureInfo*,bool all_levels);
bool load(MegustaReader*,TextureInfo*,bool all_levels);
bool load_file(const char*,TextureInfo*,bool all_levels);
}
