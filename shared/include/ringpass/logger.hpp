#pragma once

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace ringpass {

class Logger {
public:
    explicit Logger(std::filesystem::path path)
        : path_(std::move(path))
    {
        if (path_.has_parent_path())
            std::filesystem::create_directories(path_.parent_path());
    }

    void write(const std::string& line)
    {
        std::scoped_lock lock(mutex_);

        SYSTEMTIME st{};
        GetLocalTime(&st);

        std::ofstream out(path_, std::ios::app);
        if (!out)
            return;

        out << '['
            << st.wYear << '-'
            << two(st.wMonth) << '-'
            << two(st.wDay) << ' '
            << two(st.wHour) << ':'
            << two(st.wMinute) << ':'
            << two(st.wSecond) << '.'
            << three(st.wMilliseconds)
            << "] " << line << '\n';
    }

private:
    static std::string two(WORD value)
    {
        if (value < 10) return "0" + std::to_string(value);
        return std::to_string(value);
    }

    static std::string three(WORD value)
    {
        if (value < 10) return "00" + std::to_string(value);
        if (value < 100) return "0" + std::to_string(value);
        return std::to_string(value);
    }

    std::filesystem::path path_;
    std::mutex mutex_;
};

} // namespace ringpass
