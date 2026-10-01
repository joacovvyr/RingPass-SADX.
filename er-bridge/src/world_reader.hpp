#pragma once

#include <cstdint>

#include "game_views.hpp"
#include "module_scanner.hpp"

#include <ringpass/logger.hpp>
#include <ringpass/protocol.hpp>

namespace ringpass::er {

struct HostSample {
    bool playerValid{false};
    bool cameraValid{false};
    ringpass::Vec3 playerPosition{};
    ringpass::CameraState camera{};
};

class WorldReader {
public:
    bool initialize(
        const ModuleScanner& scanner,
        ringpass::Logger& logger);

    bool sample(HostSample& out) const;

private:
    static bool readable(const void* ptr, std::size_t size);
    static std::uintptr_t resolve_rip(
        std::uintptr_t instruction,
        std::size_t displacementOffset,
        std::size_t instructionLength);

    template <typename T>
    static T* read_global_pointer(std::uintptr_t slot)
    {
        if (!readable(
                reinterpret_cast<const void*>(slot),
                sizeof(T*)))
            return nullptr;

        return *reinterpret_cast<T**>(slot);
    }

    std::uintptr_t worldChrManSlot_{};
    std::uintptr_t fieldAreaSlot_{};
};

} // namespace ringpass::er
