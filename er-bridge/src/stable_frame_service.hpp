#pragma once

#include <cstdint>

#include "host_environment.hpp"

#include <ringpass/protocol.hpp>

namespace ringpass::er {

struct StableFrameSample {
    bool valid{false};
    std::uint32_t zone{};
    ringpass::Vec3 playerPosition{};
    ringpass::Vec3 havokOffset{};
};

class StableFrameService {
public:
    bool initialize(HostEnvironment& host);
    bool sample(StableFrameSample& out) const;

    [[nodiscard]] bool active() const { return active_; }

    static ringpass::Vec3 to_stable(
        ringpass::Vec3 havok,
        ringpass::Vec3 offset)
    {
        return {
            havok.x - offset.x,
            havok.y - offset.y,
            havok.z - offset.z
        };
    }

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
        return safe_read(address, &out, sizeof(T));
    }

    bool validate_vtable(
        const void* object,
        std::uintptr_t expected) const;

    std::uintptr_t base_{};
    bool active_{false};
};

} // namespace ringpass::er
