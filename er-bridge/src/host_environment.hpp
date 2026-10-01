#pragma once

#include <windows.h>

#include <filesystem>

#include "module_scanner.hpp"
#include <ringpass/logger.hpp>

namespace ringpass::er {

class HostEnvironment {
public:
    HostEnvironment();

    bool initialize();

    [[nodiscard]] bool ready() const { return ready_; }
    [[nodiscard]] const ModuleScanner& scanner() const { return scanner_; }

    ringpass::Logger& log() { return logger_; }

private:
    static std::filesystem::path log_path();

    ringpass::Logger logger_;
    ModuleScanner scanner_;
    bool ready_{false};
};

} // namespace ringpass::er
