#include "game_application.h"
#include <cstdio>
#include <exception>
int main(int argc,char** argv) {
    try {
        megusta::runGameApplication(nullptr,argc,argv);
        return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"Megusta startup failure: %s\n",error.what());
        return 1;
    }
}
