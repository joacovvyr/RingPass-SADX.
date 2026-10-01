#include <windows.h>
#include <ringpass/protocol.hpp>

// NOTE:
// This is only the bridge bootstrap. The next step is wiring this target
// to the SADX Mod Loader SDK and reading the real player entity/state.

namespace {

ringpass::CharacterState g_lastState{};

void initialize_bridge() {
    g_lastState.protocolVersion = ringpass::kProtocolVersion;
    g_lastState.character = ringpass::CharacterId::Unknown;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        initialize_bridge();
    }

    return TRUE;
}
