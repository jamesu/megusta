// Link-only fixture: CMake retains every selected library object so missing
// dependencies cannot be hidden by archive extraction or section collection.
#include "../src/engine/resource_runtime.h"
namespace megusta {
// Game-owned shared I/O pointer, normally initialized by ResourceRuntime.
// The library closure check links archives without the game runtime.
SharedIoServices* sharedIo = nullptr;
}
int main() { return 0; }
