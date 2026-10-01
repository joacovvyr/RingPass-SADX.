#pragma once

#include <cstdint>

namespace ringpass {
class Logger;
}

namespace ringpass::sadx {

class TargetProxyAdapter;

bool initialize_target_injection(
    TargetProxyAdapter* adapter,
    ringpass::Logger* logger);

void shutdown_target_injection();

[[nodiscard]] bool target_injection_enabled();

} // namespace ringpass::sadx
