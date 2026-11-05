#include "task.h"
#include "task_constants.h"
#include "Python.h"
namespace megusta {
// 0041c610: CPython 1.5.2 callback invoked with an empty positional tuple.
void invokePythonTaskCallback(std::uintptr_t value) {
    PyObject* callable=reinterpret_cast<PyObject*>(value);
    PyObject* arguments=Py_BuildValue(const_cast<char*>(task_constants::emptyTupleFormat));
    PyObject* result=PyEval_CallObjectWithKeywords(callable,arguments,nullptr);
    Py_DECREF(arguments);
    if (!result) PyErr_Print(); else Py_DECREF(result);
    if (Py_FlushLine()!=0) PyErr_Clear();
}
void releasePythonTaskCallback(std::uintptr_t value) { Py_DECREF(reinterpret_cast<PyObject*>(value)); }
}
