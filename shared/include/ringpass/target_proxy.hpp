#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include <ringpass/protocol.hpp>

namespace ringpass {

class TargetProxyBuffer {
public:
    void clear()
    {
        count_ = 0;
    }

    bool push(const TargetProxy& proxy)
    {
        if (count_ >= kMaxTargets)
            return false;

        targets_[count_++] = proxy;
        return true;
    }

    [[nodiscard]] std::uint32_t count() const
    {
        return count_;
    }

    [[nodiscard]] const std::array<TargetProxy, kMaxTargets>& data() const
    {
        return targets_;
    }

    void sort_by_distance(Vec3 origin)
    {
        auto distance_sq = [origin](const TargetProxy& p)
        {
            const float dx = p.aimPoint.x - origin.x;
            const float dy = p.aimPoint.y - origin.y;
            const float dz = p.aimPoint.z - origin.z;
            return dx * dx + dy * dy + dz * dz;
        };

        std::sort(
            targets_.begin(),
            targets_.begin() + count_,
            [&](const TargetProxy& a, const TargetProxy& b)
            {
                return distance_sq(a) < distance_sq(b);
            });
    }

private:
    std::array<TargetProxy, kMaxTargets> targets_{};
    std::uint32_t count_{0};
};

} // namespace ringpass
