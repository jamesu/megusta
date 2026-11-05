/* MG-GLPNG-003: typed, borrowed input stream for stdio and mounted resources.
 * Compatibility interface; not a recovered original C ABI. */
#ifndef MEGUSTA_READER_H
#define MEGUSTA_READER_H
#include <stddef.h>
#include <stdio.h>
typedef struct MegustaReader {
    void *context;
    size_t (*read)(void *context, void *output, size_t size, size_t count);
    int (*seek)(void *context, long offset, int origin);
} MegustaReader;
static size_t megustaReaderStdioRead(void *context, void *output, size_t size, size_t count) {
    return fread(output,size,count,(FILE *)context);
}
static int megustaReaderStdioSeek(void *context, long offset, int origin) {
    return fseek((FILE *)context,offset,origin);
}
static MegustaReader megustaReaderFromFILE(FILE *file) {
    MegustaReader reader;
    reader.context=file; reader.read=megustaReaderStdioRead; reader.seek=megustaReaderStdioSeek;
    return reader;
}
#endif
