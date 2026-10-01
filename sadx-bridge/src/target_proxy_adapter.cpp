#include "target_proxy_adapter.hpp"

#include <algorithm>
#include <cmath>

namespace ringpass::sadx {

namespace {

float distance_squared(const NJS_POINT3& a, const NJS_POINT3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

} // namespace

void TargetProxyAdapter::rebuild(
    const ErToSadxChannel& hostFrame,
    const taskwk* player)
{
    count_ = 0;

    const std::uint32_t requested =
        std::min<std::uint32_t>(hostFrame.targetCount,
                                static_cast<std::uint32_t>(kMaxTargets));

    for (std::uint32_t i = 0; i < requested; ++i)
    {
        const auto& source = hostFrame.targets[i];
        if (!source.alive || !source.targetable)
            continue;

        auto& target = targets_[count_];
        target = {};
        target.hostId = source.id;
        target.active = true;

        target.task.pos = {
            source.position.x,
            source.position.y,
            source.position.z
        };

        // SADX homing code resolves the target point from collision info.
        // Keep center relative to task position so we can represent the
        // host's preferred aim point without changing SADX selection logic.
        target.collisionInfo.center = {
            source.aimPoint.x - source.position.x,
            source.aimPoint.y - source.position.y,
            source.aimPoint.z - source.position.z
        };

        target.collisionInfo.a = source.radius;
        target.collisionInfo.b = source.radius;
        target.collisionInfo.c = source.radius;

        target.collision.info = &target.collisionInfo;
        target.collision.nbInfo = 1;
        target.collision.colli_range = source.radius;
        target.task.cwp = &target.collision;

        if (player)
        {
            // Stored for debugging and for the later list-injection stage.
            target.task.value.f =
                distance_squared(player->pos, target.task.pos);
        }

        ++count_;
    }
}

} // namespace ringpass::sadx
