#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace megusta::platform {
struct PixelRectangle { int x,y,width,height; };
struct SurfaceTransform {
    int windowWidth=0,windowHeight=0,drawableWidth=0,drawableHeight=0;
    bool enabled()const{return windowWidth>0 && windowHeight>0 && drawableWidth>0 && drawableHeight>0;}
    PixelRectangle fit(int width,int height)const {
        int w=drawableWidth,h=drawableHeight;
        if(static_cast<std::int64_t>(w)*height>static_cast<std::int64_t>(h)*width)
            w=std::max(1,static_cast<int>(static_cast<std::int64_t>(h)*width/height));
        else h=std::max(1,static_cast<int>(static_cast<std::int64_t>(w)*height/width));
        return {(drawableWidth-w)/2,(drawableHeight-h)/2,w,h};
    }
    void toCanvas(int& x,int& y,int width,int height)const {
        const auto r=fit(width,height);
        x=static_cast<int>(std::floor((double(x)*drawableWidth/windowWidth-r.x)*width/r.width));
        y=static_cast<int>(std::floor((double(y)*drawableHeight/windowHeight-(drawableHeight-r.y-r.height))*height/r.height));
    }
    PixelRectangle toPixels(int x,int y,int w,int h,int width,int height)const {
        const auto r=fit(width,height);
        const int left=std::max(r.x,static_cast<int>(std::floor(r.x+double(x)*r.width/width)));
        const int bottom=std::max(r.y,static_cast<int>(std::floor(r.y+double(y)*r.height/height)));
        const int right=std::min(r.x+r.width,static_cast<int>(std::ceil(r.x+(double(x)+w)*r.width/width)));
        const int top=std::min(r.y+r.height,static_cast<int>(std::ceil(r.y+(double(y)+h)*r.height/height)));
        return {left,bottom,std::max(0,right-left),std::max(0,top-bottom)};
    }
    void toWindow(int& x,int& y,int width,int height)const {
        const auto r=fit(width,height);
        x=static_cast<int>(std::lround((r.x+double(x)*r.width/width)*windowWidth/drawableWidth));
        y=static_cast<int>(std::lround((drawableHeight-r.y-r.height+double(y)*r.height/height)*windowHeight/drawableHeight));
    }
};
}
