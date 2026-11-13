#include "engine/resource_runtime.h"
#include "game/game_config.h"
#include "game/python_resource_import.h"
#include "runtime_setup.h"
#include "marshal.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <stdexcept>

namespace megusta::engine_runtime {
[[noreturn]] void engineRuntime_fatalExit_impl() { std::abort(); }
}
namespace {
void require(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
void write(const std::filesystem::path& path,const std::string& bytes) {
    std::ofstream file(path,std::ios::binary);file.write(bytes.data(),bytes.size());
    require(bool(file),"fixture write failed");
}
void bytecode(const std::filesystem::path& path) {
    char source[]="value=73\n",filename[]="compiled_fixture.py";
    PyObject* code=Py_CompileString(source,filename,Py_file_input);
    require(code!=nullptr,"fixture compilation failed");
    PyObject* bytes=PyMarshal_WriteObjectToString(code);Py_DECREF(code);
    require(bytes!=nullptr,"fixture marshal failed");
    std::string header(8,'\0');unsigned long magic=PyImport_GetMagicNumber();
    for(unsigned i=0;i<4;++i)header[i]=static_cast<char>(magic>>(8*i));
    header.append(PyString_AsString(bytes),PyString_Size(bytes));Py_DECREF(bytes);
    write(path,header);
}
long value(const char* name) {
    PyObject* builtin=PyDict_GetItemString(PyEval_GetBuiltins(),const_cast<char*>("__import__"));
    PyObject* module=PyObject_CallFunction(builtin,const_cast<char*>("s"),name);
    if(!module){PyErr_Print();throw std::runtime_error(name);}
    PyObject* result=PyObject_GetAttrString(module,const_cast<char*>("value"));Py_DECREF(module);
    require(result!=nullptr,"missing fixture value");long number=PyInt_AsLong(result);Py_DECREF(result);return number;
}
PyObject* importModule(const char* name) {
    return PyObject_CallFunction(PyDict_GetItemString(PyEval_GetBuiltins(),const_cast<char*>("__import__")),
        const_cast<char*>("s"),name);
}
}
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::current_path()/"python-import-fixture";
    const auto mod=root/"custom mod",base=root/"base";
    fs::create_directories(mod);fs::create_directories(base);
    write(root/"MINIGOLF.CFG","[Prefs]\r\nDefaultMod="+mod.string()+"\r\n");
    write(base/"newlines.py","value=0\n");
    write(mod/"newlines.py","# Windows text\r\nvalue=17\r\nvalue=value+1\x1a" "invalid python");
    write(base/"compiled.py","value=0\n");
    write(mod/"broken.pyo","invalid header");
    write(mod/"raises.py","raise ValueError, 'keep original failure'\r\n");
    const char* args[]={"check"};megusta::ResourceRuntime runtime;runtime.initializeFromArgs(1,args);
    require(runtime.mounts().add("data/",base.string().c_str()),"base mount");
    require(runtime.mounts().add("/mnt/memcard/",root.string().c_str()),"config mount");
    megusta::GameConfig config("/mnt/memcard/minigolf.cfg");require(config.load(runtime.mounts()),"config load");
    require(runtime.applyDefaultMod(config.readString("Prefs","DefaultMod",""),1,args),"default mod mount");
    megusta::configureGamePython();Py_Initialize();megusta::installMountedPythonImporter();
    bytecode(mod/"compiled.pyO");
    require(value("newlines")==18,"loose CRLF, Ctrl-Z and source override");
    require(value("compiled")==73,"mod bytecode must override base source");
    PyObject* module=importModule("math");require(module!=nullptr,"builtin fallback");Py_DECREF(module);
    module=importModule("broken");require(!module && PyErr_ExceptionMatches(PyExc_ImportError),"bad bytecode rejection");PyErr_Clear();
    module=importModule("raises");require(!module && PyErr_ExceptionMatches(PyExc_ValueError),"source exception propagation");PyErr_Clear();
    Py_Finalize();
    const std::string cli=base.string();const char* explicitArgs[]={"check","-MoD",cli.c_str()};
    runtime.initializeFromArgs(3,explicitArgs);
    require(runtime.applyDefaultMod(mod.string().c_str(),3,explicitArgs),"CLI override");
    megusta::configureGamePython();Py_Initialize();megusta::installMountedPythonImporter();
    require(value("newlines")==0,"explicit CLI mod wins over configured mod");Py_Finalize();
    std::puts("PASS: DefaultMod INI, CRLF/Ctrl-Z, mod bytecode priority, builtins, errors and CLI override");
}
