#include <cassert>
#include <cmath>
#include <iostream>

#include <ringpass/coordinates.hpp>
#include <ringpass/protocol.hpp>
#include <ringpass/target_proxy.hpp>

namespace {

bool near(float a, float b, float eps = 0.0001f)
{
    return std::fabs(a - b) <= eps;
}

void test_coordinate_roundtrip()
{
    ringpass::CoordinateTransform tx{};
    tx.scale = 2.5f;
    tx.sadxOrigin = { 10.0f, 2.0f, -4.0f };
    tx.erOrigin = { -100.0f, 50.0f, 25.0f };
    tx.swapYZ = true;
    tx.invertX = true;
    tx.invertZ = true;

    const ringpass::Vec3 input{ 12.0f, 7.0f, 9.0f };
    const auto er = tx.sadx_to_er(input);
    const auto output = tx.er_to_sadx(er);

    assert(near(input.x, output.x));
    assert(near(input.y, output.y));
    assert(near(input.z, output.z));
}

void test_target_sort()
{
    ringpass::TargetProxyBuffer buffer;

    ringpass::TargetProxy far{};
    far.id = 2;
    far.aimPoint = { 20.0f, 0.0f, 0.0f };

    ringpass::TargetProxy nearTarget{};
    nearTarget.id = 1;
    nearTarget.aimPoint = { 2.0f, 0.0f, 0.0f };

    assert(buffer.push(far));
    assert(buffer.push(nearTarget));
    buffer.sort_by_distance({});

    assert(buffer.count() == 2);
    assert(buffer.data()[0].id == 1);
    assert(buffer.data()[1].id == 2);
}

void test_protocol_defaults()
{
    ringpass::SharedState state{};
    assert(state.magic == ringpass::kSharedMagic);
    assert(state.protocolVersion == ringpass::kProtocolVersion);
    assert(state.sadx.protocolVersion == ringpass::kProtocolVersion);
    assert(state.er.protocolVersion == ringpass::kProtocolVersion);
}

} // namespace

int main()
{
    test_coordinate_roundtrip();
    test_target_sort();
    test_protocol_defaults();

    std::cout << "RingPass protocol tests passed.\n";
    return 0;
}
