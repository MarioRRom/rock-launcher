#include "rocklaunch/core/logger.h"

#include "rocklaunch/core/config_store.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace rocklaunch
{

namespace
{

constexpr std::uintmax_t kMaxLogSize = 100 * 1024; // 100 KB
constexpr int kMaxRotatedFiles = 2; // keep .1 and .2

std::string Timestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime {};
    localtime_r(&now, &localTime);

    std::ostringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return timestamp.str();
}

// ANSI color for a log level, applied to the level token on the console only.
std::string LevelColor(std::string_view level)
{
    if (level == "DEBUG") {
        return "\033[36m";
    }
    if (level == "INFO") {
        return "\033[32m";
    }
    if (level == "WARN") {
        return "\033[33m";
    }
    if (level == "ERROR") {
        return "\033[31m";
    }
    return "\033[0m";
}

void RotateLog(const fs::path &logFile)
{
    std::error_code ec;
    if (!fs::is_regular_file(logFile, ec)) {
        return;
    }

    if (fs::file_size(logFile, ec) <= kMaxLogSize) {
        return;
    }

    // Remove the oldest rotated file.
    fs::path oldest = logFile.string() + "." + std::to_string(kMaxRotatedFiles);
    fs::remove(oldest, ec);

    // Shift: .1 -> .2, .log -> .1
    for (int i = kMaxRotatedFiles - 1; i >= 1; --i) {
        fs::path from = logFile.string() + "." + std::to_string(i);
        fs::path to = logFile.string() + "." + std::to_string(i + 1);
        fs::rename(from, to, ec);
    }

    fs::path first = logFile.string() + ".1";
    fs::rename(logFile, first, ec);
}

} // namespace

Logger::Logger(fs::path logDir)
    : m_logFile(std::move(logDir) / "rocklaunch.log")
{
    // Best effort: a logger that cannot start must not abort its caller, and
    // every message still reaches std::cerr.
    std::error_code error;
    fs::create_directories(m_logFile.parent_path(), error);
}

void Logger::Debug(std::string_view message) const
{
    Write("DEBUG", message);
}

void Logger::Info(std::string_view message) const
{
    Write("INFO", message);
}

void Logger::Warn(std::string_view message) const
{
    Write("WARN", message);
}

void Logger::Error(std::string_view message) const
{
    Write("ERROR", message);
}

fs::path Logger::LogFile() const
{
    return m_logFile;
}

fs::path Logger::DefaultLogDir()
{
    return ConfigStore::DefaultDataDir() / "logs";
}

void Logger::Write(std::string_view level, std::string_view message) const
{
    // Rotation renames the log and the append reopens it, so the whole write is
    // locked: std::cerr does not race, but it does interleave mid-line.
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);

    std::cerr << "[" << LevelColor(level) << level << "\033[0m] " << message << '\n';

    if (level == "DEBUG") {
        return;
    }

    std::string timestamp = Timestamp();
    std::string fileLine = timestamp + " [" + std::string(level) + "] " + std::string(message);

    RotateLog(m_logFile);

    std::ofstream output(m_logFile, std::ios::app);
    // Best effort, and never an exception: whatever is being logged about
    // has usually already succeeded, so a full disk is not its failure.
    if (!output.is_open()) {
        // Static: Logger is built per message, so a member flag would re-report every time.
        static bool reported = false;
        if (!reported) {
            reported = true;
            std::cerr << "[" << LevelColor("ERROR") << "ERROR" << "\033[0m] "
                      << "cannot write the logfile " << m_logFile << '\n';
        }
        return;
    }

    output << fileLine << '\n';
}

} // namespace rocklaunch
