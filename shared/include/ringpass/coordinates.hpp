#pragma once

#include <ringpass/protocol.hpp>

namespace ringpass {

struct CoordinateTransform {
    float scale{1.0f};
    Vec3 sadxOrigin{};
    Vec3 erOrigin{};
    bool swapYZ{false};
    bool invertX{false};
    bool invertZ{false};

    Vec3 sadx_to_er(Vec3 p) const
    {
        p.x -= sadxOrigin.x;
        p.y -= sadxOrigin.y;
        p.z -= sadxOrigin.z;

        if (swapYZ)
        {
            const float y = p.y;
            p.y = p.z;
            p.z = y;
        }

        if (invertX) p.x = -p.x;
        if (invertZ) p.z = -p.z;

        p.x *= scale;
        p.y *= scale;
        p.z *= scale;

        p.x += erOrigin.x;
        p.y += erOrigin.y;
        p.z += erOrigin.z;
        return p;
    }

    Vec3 er_to_sadx(Vec3 p) const
    {
        p.x -= erOrigin.x;
        p.y -= erOrigin.y;
        p.z -= erOrigin.z;

        p.x /= scale;
        p.y /= scale;
        p.z /= scale;

        if (invertX) p.x = -p.x;
        if (invertZ) p.z = -p.z;

        if (swapYZ)
        {
            const float y = p.y;
            p.y = p.z;
            p.z = y;
        }

        p.x += sadxOrigin.x;
        p.y += sadxOrigin.y;
        p.z += sadxOrigin.z;
        return p;
    }
};

} // namespace ringpass
