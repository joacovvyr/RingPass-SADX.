#include <windows.h>

#include <cassert>
#include <cmath>
#include <iostream>

#include <ringpass/coordinates.hpp>
#include <ringpass/health.hpp>
#include <ringpass/protocol.hpp>
#include <ringpass/target_proxy.hpp>

namespace {

bool approx_equal(float a, float b, float eps = 0.0001f)
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

    assert(approx_equal(input.x, output.x));
    assert(approx_equal(input.y, output.y));
    assert(approx_equal(input.z, output.z));
}

void test_target_sort()
{
    ringpass::TargetProxyBuffer buffer;

    ringpass::TargetProxy farTarget{};
    farTarget.id = 2;
    farTarget.aimPoint = { 20.0f, 0.0f, 0.0f };

    ringpass::TargetProxy closeTarget{};
    closeTarget.id = 1;
    closeTarget.aimPoint = { 2.0f, 0.0f, 0.0f };

    assert(buffer.push(farTarget));
    assert(buffer.push(closeTarget));
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
    assert(sizeof(ringpass::SharedState) == 5840);
}

void test_channel_health()
{
    const auto now = GetTickCount64();

    assert(ringpass::channel_health(
        ringpass::kProtocolVersion, now) ==
        ringpass::ChannelHealth::Live);

    const auto stale = now > (ringpass::kHeartbeatTimeoutMs + 100)
        ? now - ringpass::kHeartbeatTimeoutMs - 100
        : 1;

    if (now > ringpass::kHeartbeatTimeoutMs + 100)
    {
        assert(ringpass::channel_health(
            ringpass::kProtocolVersion, stale) ==
            ringpass::ChannelHealth::Stale);
    }

    assert(ringpass::channel_health(
        ringpass::kProtocolVersion + 1, now) ==
        ringpass::ChannelHealth::Incompatible);

    assert(ringpass::channel_health(
        ringpass::kProtocolVersion, 0) ==
        ringpass::ChannelHealth::Offline);
}

} // namespace

int main()
{
    test_coordinate_roundtrip();
    test_target_sort();
    test_protocol_defaults();
    test_channel_health();

    std::cout << "RingPass protocol tests passed.\n";
    return 0;
}
