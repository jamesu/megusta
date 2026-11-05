#include <strings.h>
namespace megusta { namespace strings {
int compareInsensitive(const char* left,const char* right) { return ::strcasecmp(left,right); }
} }
