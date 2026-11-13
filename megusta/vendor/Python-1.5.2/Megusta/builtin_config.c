/* Built-in module table reconstructed from the game's recovered CPython inittab. */
#include "Python.h"

extern void initarray(void);
extern void initbinascii(void);
extern void initcmath(void);
extern void initerrno(void);
extern void initmath(void);
extern void initmd5(void);
extern void initnew(void);
extern void initoperator(void);
extern void initregex(void);
extern void initrotor(void);
extern void initsha(void);
#ifdef _WIN32
extern void init_socket(void);
#elif !defined(__EMSCRIPTEN__)
extern void initsocket(void);
#endif
extern void initsoundex(void);
extern void initstrop(void);
extern void initstruct(void);
extern void inittime(void);
extern void initcStringIO(void);
extern void initcPickle(void);
extern void PyMarshal_Init(void);
extern void initimp(void);

struct _inittab _PyImport_Inittab[] = {
    {"array", initarray},
    {"binascii", initbinascii},
    {"cmath", initcmath},
    {"errno", initerrno},
    {"math", initmath},
    {"md5", initmd5},
    {"new", initnew},
    {"operator", initoperator},
    {"regex", initregex},
    {"rotor", initrotor},
    {"sha", initsha},
#ifdef _WIN32
    {"_socket", init_socket},
#elif !defined(__EMSCRIPTEN__)
    {"socket", initsocket},
#endif
    {"soundex", initsoundex},
    {"strop", initstrop},
    {"struct", initstruct},
    {"time", inittime},
    {"cStringIO", initcStringIO},
    {"cPickle", initcPickle},
    {"marshal", PyMarshal_Init},
    {"imp", initimp},
    {"__main__", NULL},
    {"__builtin__", NULL},
    {"sys", NULL},
    {NULL, NULL}
};
