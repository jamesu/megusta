#include "python_resource_import.h"
#include "engine/resource_runtime.h"
#include "Python.h"
#include <string>
#include <cstring>
namespace megusta {
namespace {
struct MountedFile { PyObject_HEAD ResourceFile* file; PyObject* name; PyObject* mode; };
PyTypeObject mountedFileType={PyObject_HEAD_INIT(nullptr) 0};
PyObject* nothing() { Py_INCREF(Py_None);return Py_None; }
ResourceFile* stream(PyObject* self) {
    auto* file=reinterpret_cast<MountedFile*>(self)->file;
    if(!file)PyErr_SetString(PyExc_ValueError,const_cast<char*>("I/O operation on closed file"));
    return file;
}
PyObject* fileClose(PyObject* self,PyObject* args) {
    if(!PyArg_ParseTuple(args,""))return nullptr;
    auto* f=reinterpret_cast<MountedFile*>(self);delete f->file;f->file=nullptr;return nothing();
}
PyObject* fileRead(PyObject* self,PyObject* args) {
    int limit=-1;if(!PyArg_ParseTuple(args,"|i",&limit))return nullptr;
    auto* f=stream(self);if(!f)return nullptr;
    std::string result;char block[4096];
    while(limit<0 || static_cast<int>(result.size())<limit) {
        auto wanted=limit<0?sizeof(block):static_cast<std::size_t>(limit-result.size());
        if(wanted>sizeof(block))wanted=sizeof(block);
        auto count=f->read(block,1,static_cast<std::uint32_t>(wanted));if(!count)break;
        result.append(block,count);
    }
    return PyString_FromStringAndSize(result.data(),result.size());
}
PyObject* fileReadline(PyObject* self,PyObject* args) {
    int limit=-1;if(!PyArg_ParseTuple(args,"|i",&limit))return nullptr;
    auto* f=stream(self);if(!f)return nullptr;
    std::string result;
    while(limit<0 || static_cast<int>(result.size())<limit) {
        int value=f->getc();if(value==EOF)break;result+=static_cast<char>(value);if(value=='\n')break;
    }
    return PyString_FromStringAndSize(result.data(),result.size());
}
PyObject* fileReadlines(PyObject* self,PyObject* args) {
    int hint=0;if(!PyArg_ParseTuple(args,"|i",&hint) || !stream(self))return nullptr;
    PyObject* result=PyList_New(0);PyObject* empty=PyTuple_New(0);int total=0;
    for(;;) {
        PyObject* line=fileReadline(self,empty);if(!line){Py_DECREF(empty);Py_DECREF(result);return nullptr;}
        int size=PyString_Size(line);if(size)PyList_Append(result,line);Py_DECREF(line);total+=size;
        if(!size || (hint>0 && total>=hint))break;
    }
    Py_DECREF(empty);return result;
}
PyObject* fileWrite(PyObject* self,PyObject* args) {
    char* bytes;int size;if(!PyArg_ParseTuple(args,"s#",&bytes,&size))return nullptr;
    auto* f=stream(self);if(!f)return nullptr;
    if(f->write(bytes,1,size)!=static_cast<unsigned>(size)) { PyErr_SetString(PyExc_IOError,const_cast<char*>("mounted file write failed"));return nullptr; }
    return nothing();
}
PyObject* fileSeek(PyObject* self,PyObject* args) {
    int offset,origin=0;if(!PyArg_ParseTuple(args,"i|i",&offset,&origin))return nullptr;
    auto* f=stream(self);if(!f)return nullptr;
    if(f->seek(offset,origin)!=0){PyErr_SetString(PyExc_IOError,const_cast<char*>("mounted file seek failed"));return nullptr;}return nothing();
}
PyObject* fileTell(PyObject* self,PyObject* args) { if(!PyArg_ParseTuple(args,"") || !stream(self))return nullptr;return PyInt_FromLong(stream(self)->tell()); }
PyObject* fileFlush(PyObject* self,PyObject* args) { if(!PyArg_ParseTuple(args,"") || !stream(self))return nullptr;stream(self)->flush();return nothing(); }
PyObject* fileIsatty(PyObject* self,PyObject* args) { if(!PyArg_ParseTuple(args,"") || !stream(self))return nullptr;return PyInt_FromLong(stream(self)->isatty()); }
PyMethodDef fileMethods[]={
    {const_cast<char*>("close"),fileClose,METH_VARARGS,nullptr},
    {const_cast<char*>("read"),fileRead,METH_VARARGS,nullptr},
    {const_cast<char*>("readline"),fileReadline,METH_VARARGS,nullptr},
    {const_cast<char*>("readlines"),fileReadlines,METH_VARARGS,nullptr},
    {const_cast<char*>("write"),fileWrite,METH_VARARGS,nullptr},
    {const_cast<char*>("seek"),fileSeek,METH_VARARGS,nullptr},
    {const_cast<char*>("tell"),fileTell,METH_VARARGS,nullptr},
    {const_cast<char*>("flush"),fileFlush,METH_VARARGS,nullptr},
    {const_cast<char*>("isatty"),fileIsatty,METH_VARARGS,nullptr},{nullptr,nullptr,0,nullptr}};
void fileDestroy(PyObject* self) { auto* f=reinterpret_cast<MountedFile*>(self);delete f->file;Py_XDECREF(f->name);Py_XDECREF(f->mode);PyMem_DEL(self); }
PyObject* fileAttribute(PyObject* self,char* name) {
    auto* f=reinterpret_cast<MountedFile*>(self);
    if(!std::strcmp(name,"closed"))return PyInt_FromLong(f->file==nullptr);
    PyObject* value=!std::strcmp(name,"name")?f->name:!std::strcmp(name,"mode")?f->mode:nullptr;
    if(value){Py_INCREF(value);return value;}return Py_FindMethod(fileMethods,self,name);
}
PyObject* mountedOpen(PyObject* stockOpen,PyObject* args) {
    char* name;char* mode=const_cast<char*>("r");int buffering=-1;
    if(!PyArg_ParseTuple(args,"s|si:open",&name,&mode,&buffering))return nullptr;
    ResourceFile* file=sharedIo?sharedIo->open(name,mode):nullptr;
    if(!file)return PyObject_CallObject(stockOpen,args);
    auto* result=PyObject_NEW(MountedFile,&mountedFileType);if(!result){delete file;return nullptr;}
    result->file=file;result->name=PyString_FromString(name);result->mode=PyString_FromString(mode);
    if(!result->name || !result->mode){Py_DECREF(reinterpret_cast<PyObject*>(result));return nullptr;}
    return reinterpret_cast<PyObject*>(result);
}
PyMethodDef openMethod={const_cast<char*>("open"),mountedOpen,METH_VARARGS,nullptr};
PyObject* mountedImport(PyObject* stockImport,PyObject* args) {
    char* name=nullptr;PyObject *globals=nullptr,*locals=nullptr,*from=nullptr;
    if(!PyArg_ParseTuple(args,"s|OOO:__import__",&name,&globals,&locals,&from))return nullptr;
    PyObject* loaded=PyDict_GetItemString(PyImport_GetModuleDict(),name);
    if(loaded) { Py_INCREF(loaded);return loaded; }
    std::string path="data/";path+=name;path+=".py";
    // CPython's string parser expects normalized newlines. The mounted text
    // stream provides the original Windows CRT CRLF/Ctrl-Z translation.
    ResourceFile* file=sharedIo?sharedIo->open(path.c_str(),"rt"):nullptr;
    if(!file)return PyObject_CallObject(stockImport,args);
    std::string source;char block[4096];std::uint32_t count;
    while((count=file->read(block,1,sizeof(block)))!=0)source.append(block,count);
    sharedIo->close(file);
    // File parsing accepts an unterminated final line; Py_CompileString in
    // Python 1.5.2 requires the equivalent terminating NEWLINE token.
    if(source.empty() || source.back()!='\n')source+='\n';
    PyObject* code=Py_CompileString(const_cast<char*>(source.c_str()),const_cast<char*>(path.c_str()),Py_file_input);
    if(!code)return nullptr;
    PyObject* module=PyImport_ExecCodeModule(name,code);Py_DECREF(code);return module;
}
PyMethodDef importMethod={const_cast<char*>("__import__"),mountedImport,METH_VARARGS,nullptr};
}
void installMountedPythonImporter() {
    PyObject* builtins=PyEval_GetBuiltins();
    PyObject* stockImport=PyDict_GetItemString(builtins,const_cast<char*>("__import__"));
    PyObject* replacement=PyCFunction_New(&importMethod,stockImport);
    if(replacement) { PyDict_SetItemString(builtins,const_cast<char*>("__import__"),replacement);Py_DECREF(replacement); }
    mountedFileType.ob_type=&PyType_Type;
    mountedFileType.tp_name=const_cast<char*>("mounted_file");
    mountedFileType.tp_basicsize=sizeof(MountedFile);
    mountedFileType.tp_dealloc=fileDestroy;mountedFileType.tp_getattr=fileAttribute;
    PyObject* stockOpen=PyDict_GetItemString(builtins,const_cast<char*>("open"));
    replacement=PyCFunction_New(&openMethod,stockOpen);
    if(replacement){PyDict_SetItemString(builtins,const_cast<char*>("open"),replacement);Py_DECREF(replacement);}
}
}
