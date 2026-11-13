// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include <memory>
namespace megusta::engine_movie {
struct MovieHost { void* window; bool graphicsEnabled; };
class MoviePlayback {
public:
    void openAvi(const char* filename);
    void decodeAndUploadFrame(int frame);
    void closeAvi();
    bool initializeTiled(const char* filename,void* window);
    void releaseTiled();
    void advanceTime(int milliseconds);
    void drawTiledFrame();
    int playBlocking(const MovieHost&,const char*,unsigned flags,int x,int y,int width,int height);
    int width=0,height=0,destinationX=0,destinationY=0,destinationWidth=0,destinationHeight=0;
    int frames=0,millisecondsPerFrame=0,elapsed=0,currentFrame=0;
    bool finished=false;
private:
    void* native_=nullptr;
    int rows_=0,columns_=0,tileCount_=0,previousViewport_[4]{};
    std::unique_ptr<std::uint32_t[]> textures_;
    std::unique_ptr<int[]> tileWidths_,tileHeights_;
};
void bindMovieHost(const MovieHost&);
int playMovie(const char*,unsigned,int,int,int,int);
bool registerMovieCleanup();
}
