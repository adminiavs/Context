#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <fstream>

namespace Ragger {

// Simple logging utility that works without external dependencies
class Logger {
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    Logger();
    ~Logger() = default;

    // Initialization
    void initialize(const std::string& logFile = "");
    void setLevel(Level level);

    // Logging methods
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

    // Context logging (with plugin/component context)
    void logWithContext(Level level, const std::string& context, const std::string& message);

    // Performance logging
    void logPerformance(const std::string& operation, long long durationMs);
    void logMemoryUsage(const std::string& context);

    // Error logging with stack trace (if available)
    void logErrorWithContext(const std::string& context, const std::string& error,
                           const std::string& stackTrace = "");

    // Flush logs
    void flush();

private:
    Level currentLevel_;
    std::ofstream fileStream_;

    void logMessage(Level level, const std::string& message);
    std::string getTimestamp() const;
    std::string levelToString(Level level) const;
    std::string formatContext(const std::string& context) const;
};

} // namespace Ragger
