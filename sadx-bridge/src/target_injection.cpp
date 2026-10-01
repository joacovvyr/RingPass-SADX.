#include "target_injection.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "FunctionHook.h"
#include "SADXModLoader.h"
#include "target_proxy_adapter.hpp"

#include <ringpass/logger.hpp>

namespace ringpass::sadx {

namespace {

constexpr std::uintptr_t kCCLAnalyzeAddress =
    0x00420700;

constexpr std::uint16_t kMaxNativeEnemyTargets =
    656;

TargetProxyAdapter* g_adapter = nullptr;
ringpass::Logger* g_logger = nullptr;

std::unique_ptr<FunctionHook<void>>
    g_cclAnalyzeHook;

void log(const std::string& line)
{
    if (g_logger)
        g_logger->write(line);
}

void append_prepared_targets()
{
    if (!g_adapter ||
        !playertwp[0])
        return;

    std::uint16_t count =
        ael_num0;

    if (count > kMaxNativeEnemyTargets)
        count = kMaxNativeEnemyTargets;

    for (std::uint32_t i = 0;
         i < g_adapter->count() &&
         count < kMaxNativeEnemyTargets;
         ++i)
    {
        auto* synthetic =
            g_adapter->target(i);

        if (!synthetic ||
            !synthetic->active ||
            !synthetic->task.cwp)
            continue;

        const float distanceSquared =
            synthetic->distanceSquared;

        if (!std::isfinite(
                distanceSquared) ||
            distanceSquared < 0.0f)
            continue;

        around_enemy_list_p0[count].twp =
            &synthetic->task;

        around_enemy_list_p0[count].dist =
            distanceSquared;

        ++count;
    }

    ael_num0 = count;

    // Native lists are sentinel-terminated.
    around_enemy_list_p0[count].twp =
        nullptr;
}

void CCLAnalyzeHook()
{
    g_cclAnalyzeHook->Original();

    // Native collision analysis has just rebuilt the target list.
    // Append RingPass proxies here so SADX's own character code sees them.
    append_prepared_targets();
}

} // namespace

bool initialize_target_injection(
    TargetProxyAdapter* adapter,
    ringpass::Logger* logger)
{
    if (g_cclAnalyzeHook)
        return true;

    if (!adapter)
        return false;

    g_adapter = adapter;
    g_logger = logger;

    try
    {
        g_cclAnalyzeHook =
            std::make_unique<
                FunctionHook<void>>(
                    kCCLAnalyzeAddress,
                    CCLAnalyzeHook);
    }
    catch (...)
    {
        g_cclAnalyzeHook.reset();
        g_adapter = nullptr;
        log(
            "failed to install experimental "
            "CCL_Analyze target injection hook");
        return false;
    }

    log(
        "EXPERIMENTAL target injection enabled: "
        "RingPass proxies append after CCL_Analyze");

    return true;
}

void shutdown_target_injection()
{
    // FunctionHook restores the original bytes when its owning object
    // is destroyed only if its implementation supports that lifecycle.
    // We intentionally retain the hook for the process lifetime and
    // only clear external pointers during shutdown.
    g_adapter = nullptr;
    g_logger = nullptr;
}

bool target_injection_enabled()
{
    return g_cclAnalyzeHook != nullptr;
}

} // namespace ringpass::sadx
