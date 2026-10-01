#pragma once

#include <array>
#include <cstdint>

#include "SADXModLoader.h"
#include <ringpass/protocol.hpp>

namespace ringpass::sadx {

struct SyntheticTarget {
    taskwk task{};
    colliwk collision{};
    CCL_INFO collisionInfo{};
    std::uint64_t hostId{};
    bool active{false};
};

class TargetProxyAdapter {
public:
    void rebuild(const ErToSadxChannel& hostFrame, const taskwk* player);

    [[nodiscard]] std::uint32_t count() const { return count_; }
    [[nodiscard]] SyntheticTarget* target(std::uint32_t index)
    {
        return index < count_ ? &targets_[index] : nullptr;
    }

private:
    std::array<SyntheticTarget, kMaxTargets> targets_{};
    std::uint32_t count_{0};
};

} // namespace ringpass::sadx
