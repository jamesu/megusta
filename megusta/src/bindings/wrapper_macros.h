// Intentionally reincludable: method-list expressions are model authored.
#define SWIG_BEGIN(T,N,F,DECL,...) static PyObject* T##_##N(PyObject*,PyObject* args) { \
    char* encoded; DECL; if (!PyArg_ParseTuple(args,const_cast<char*>(F ":" #T "_" #N),&encoded, ##__VA_ARGS__)) return nullptr; \
    auto* p=receiver<T>(encoded,"_" #T "_p",#T "_" #N); if (!p && PyErr_Occurred()) return nullptr;
#define SIMPLE(T,N,E) SWIG_BEGIN(T,N,"s",) E; }
#define INTEGER(T,N,E) SIMPLE(T,N,return Py_BuildValue(const_cast<char*>("i"),static_cast<int>(E)))
#define FLOATING(T,N,E) SIMPLE(T,N,return Py_BuildValue(const_cast<char*>("f"),static_cast<double>(E)))
#define TEXTUAL(T,N,E) SIMPLE(T,N,return Py_BuildValue(const_cast<char*>("s"),E))
#define POINTER(T,N,E,R) SIMPLE(T,N,return pointerResult(E,"_" #R "_p"))
#define SETINT(T,N,E) SWIG_BEGIN(T,N,"si",int value,&value) E; return none(); }
#define NEED_GAMEPLAY if (!gameplayServices()) return missingGameplayServices();
