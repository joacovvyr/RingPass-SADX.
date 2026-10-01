#include <windows.h>
#include <ringpass/protocol.hpp>

// Elden Ring host adapter bootstrap.
//
// Keep this module isolated from the shared protocol so host-specific hooks,
// render integration and Seamless compatibility can evolve independently.

namespace {

void initialize_er_bridge() {
    // TODO:
    // 1. validate supported game build
    // 2. initialize IPC
    // 3. initialize read-only world probes
    // 4. initialize camera/render adapter
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        initialize_er_bridge();
    }

    return TRUE;
}
