#pragma once

#include <cstdint>

#include "host_environment.hpp"

#include <ringpass/protocol.hpp>

namespace ringpass::er {

class EnemyReader {
public:
    bool initialize(HostEnvironment& host);

    std::uint32_t sample(
        ringpass::Vec3 havokOffset,
        ringpass::TargetProxy* out,
        std::uint32_t capacity) const;

    [[nodiscard]] bool active() const { return active_; }

private:
    static bool safe_read(
        const void* address,
        void* out,
        std::size_t size);

    template <typename T>
    static bool read_value(
        const void* address,
        T& out)
    {
        return safe_read(
            address,
            &out,
            sizeof(T));
    }

    bool validate_vtable(
        const void* object,
        std::uintptr_t expected) const;

    bool active_{false};
    std::uintptr_t base_{};
};

} // namespace ringpass::er
