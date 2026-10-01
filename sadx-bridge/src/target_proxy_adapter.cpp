#include "target_proxy_adapter.hpp"

#include <algorithm>
#include <cmath>

namespace ringpass::sadx {

namespace {

float distance_squared(
    const NJS_POINT3& a,
    const NJS_POINT3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

NJS_POINT3 to_njs(Vec3 value)
{
    return {
        value.x,
        value.y,
        value.z
    };
}

} // namespace

void TargetProxyAdapter::clear()
{
    count_ = 0;
}

void TargetProxyAdapter::rebuild(
    const ErToSadxChannel& hostFrame,
    const taskwk* player,
    const CoordinateTransform& transform)
{
    count_ = 0;

    const std::uint32_t requested =
        std::min<std::uint32_t>(
            hostFrame.targetCount,
            static_cast<std::uint32_t>(
                kMaxTargets));

    for (std::uint32_t i = 0;
         i < requested;
         ++i)
    {
        const auto& source =
            hostFrame.targets[i];

        if (!source.alive ||
            !source.targetable)
            continue;

        const Vec3 mappedPosition =
            transform.er_to_sadx(
                source.position);

        const Vec3 mappedAimPoint =
            transform.er_to_sadx(
                source.aimPoint);

        auto& target =
            targets_[count_];

        target = {};
        target.hostId = source.id;
        target.active = true;

        target.task.pos =
            to_njs(mappedPosition);

        // The native homing code resolves an entity target point as
        // task position + collision-info center when attr 0x20 is clear.
        target.collisionInfo.attr = 0;
        target.collisionInfo.center =
        {
            mappedAimPoint.x -
                mappedPosition.x,
            mappedAimPoint.y -
                mappedPosition.y,
            mappedAimPoint.z -
                mappedPosition.z
        };

        // Conservative spherical proxy. Exact shape is not used to
        // select a RingPass target; SADX still owns selection logic.
        const float mappedRadius =
            std::max(
                source.radius /
                    std::max(
                        transform.scale,
                        0.001f),
                0.05f);

        target.collisionInfo.a =
            mappedRadius;
        target.collisionInfo.b =
            mappedRadius;
        target.collisionInfo.c =
            mappedRadius;

        target.collision.flag = 0x40;
        target.collision.nbInfo = 1;
        target.collision.colli_range =
            mappedRadius;
        target.collision.info =
            &target.collisionInfo;

        target.task.cwp =
            &target.collision;

        if (player)
        {
            target.distanceSquared =
                distance_squared(
                    player->pos,
                    target.task.pos);

            // Keep a copy in generic task storage only for diagnostics.
            target.task.value.f =
                target.distanceSquared;
        }

        ++count_;
    }
}

} // namespace ringpass::sadx
