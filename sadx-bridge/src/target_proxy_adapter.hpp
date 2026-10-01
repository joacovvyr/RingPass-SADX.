#pragma once

#include <array>
#include <cstdint>

#include "SADXModLoader.h"

#include <ringpass/coordinates.hpp>
#include <ringpass/protocol.hpp>

namespace ringpass::sadx {

struct SyntheticTarget {
    taskwk task{};
    colliwk collision{};
    CCL_INFO collisionInfo{};
    std::uint64_t hostId{};
    float distanceSquared{};
    bool active{false};
};

class TargetProxyAdapter {
public:
    void clear();

    void rebuild(
        const ErToSadxChannel& hostFrame,
        const taskwk* player,
        const CoordinateTransform& transform);

    [[nodiscard]] std::uint32_t count() const
    {
        return count_;
    }

    [[nodiscard]] SyntheticTarget* target(
        std::uint32_t index)
    {
        return index < count_
            ? &targets_[index]
            : nullptr;
    }

    [[nodiscard]] const SyntheticTarget* target(
        std::uint32_t index) const
    {
        return index < count_
            ? &targets_[index]
            : nullptr;
    }

private:
    std::array<SyntheticTarget, kMaxTargets> targets_{};
    std::uint32_t count_{0};
};

} // namespace ringpass::sadx
