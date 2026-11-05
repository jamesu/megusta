/* Executable-evidenced disabled interrupt hooks, not the stock signal module.
 * Py_Initialize/Py_Finalize call the saved no-op service stub at the interrupt
 * initialization/finalization sites. PyObject_Print and errno handling call
 * the shared return-zero stub at their PyErr_CheckSignals sites.
 * Evidence and hashes: manifests/python_build_scope.json.
 */
#include "Python.h"
void PyOS_InitInterrupts(void) {}
void PyOS_FiniInterrupts(void) {}
int PyErr_CheckSignals(void) { return 0; }
int PyOS_InterruptOccurred(void) { return 0; }
