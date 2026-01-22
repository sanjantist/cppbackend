#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

inline constexpr std::string LOG_DIR = "/var/log/";

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << "sample_log_";
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return oss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

   public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template <class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lg(m_);

        // timestamp
        const auto now = manual_ts_.value_or(std::chrono::system_clock::now());
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << GetTimeStamp() << ": ";

        // args...
        ((oss << args), ...);

        // перевод строки, чтобы flush/строка была целиком
        oss << '\n';

        const std::string path = LOG_DIR + GetFileTimeStamp() + ".log";
        std::ofstream ofs(path, std::ios::out | std::ios::app);
        if (!ofs) {
            throw std::runtime_error("can't open file");
        }

        ofs << oss.str();
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> _(m_);
        manual_ts_ = ts;
    }

   private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex m_;
};
