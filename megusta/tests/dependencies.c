#include <stdio.h>
#include <png.h>
#include <zlib.h>
int main(void) {
    printf("Megusta dependency probe: pointer=%u zlib=%s libpng=%s\n",
           (unsigned)sizeof(void *), zlibVersion(), png_get_libpng_ver(NULL));
    return sizeof(void *) == 4 ? 0 : 1;
}
