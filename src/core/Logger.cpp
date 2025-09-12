#include "Logger.h"
#include <fstream>

namespace Ragger {

Logger::Logger() : currentLevel_(Level::Info) {
}

void Logger::initialize(const std::string& logFile) {
    if (!logFile.empty()) {
        fileStream_.open(logFile, std::ios::app);
        if (!fileStream_.is_open()) {
            std::cerr << "Failed to open log file: " << logFile << std::endl;
        }
    }
    info("Logger initialized successfully");
}

void Logger::setLevel(Level level) {
    currentLevel_ = level;
}

void Logger::trace(const std::string& message) {
    logMessage(Level::Trace, message);
}

void Logger::debug(const std::string& message) {
    logMessage(Level::Debug, message);
}

void Logger::info(const std::string& message) {
    logMessage(Level::Info, message);
}

void Logger::warning(const std::string& message) {
    logMessage(Level::Warning, message);
}

void Logger::error(const std::string& message) {
    logMessage(Level::Error, message);
}

void Logger::critical(const std::string& message) {
    logMessage(Level::Critical, message);
}

void Logger::logWithContext(Level level, const std::string& context, const std::string& message) {
    std::string formattedMessage = formatContext(context) + message;
    logMessage(level, formattedMessage);
}

void Logger::logPerformance(const std::string& operation, long long durationMs) {
    if (durationMs > 1000) {
        warning("Performance: " + operation + " took " + std::to_string(durationMs) + "ms (slow)");
    } else if (durationMs > 100) {
        info("Performance: " + operation + " took " + std::to_string(durationMs) + "ms");
    } else {
        debug("Performance: " + operation + " took " + std::to_string(durationMs) + "ms");
    }
}

void Logger::logMemoryUsage(const std::string& context) {
    debug("Memory: " + context + " - monitoring active");
}

void Logger::logErrorWithContext(const std::string& context, const std::string& error,
                               const std::string& stackTrace) {
    std::string fullMessage = formatContext(context) + "Error: " + error;
    if (!stackTrace.empty()) {
        fullMessage += "\nStack trace:\n" + stackTrace;
    }
    this->error(fullMessage);
}

void Logger::flush() {
    std::cout << std::flush;
    if (fileStream_.is_open()) {
        fileStream_ << std::flush;
    }
}

void Logger::logMessage(Level level, const std::string& message) {
    if (level < currentLevel_) {
        return;
    }

    std::string formattedMessage = getTimestamp() + " [" + levelToString(level) + "] " + message;

    // Console output
    std::cout << formattedMessage << std::endl;

    // File output
    if (fileStream_.is_open()) {
        fileStream_ << formattedMessage << std::endl;
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::levelToString(Level level) const {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error: return "ERROR";
        case Level::Critical: return "CRIT";
        default: return "UNKNOWN";
    }
}

std::string Logger::formatContext(const std::string& context) const {
    if (context.empty()) {
        return "";
    }
    return "[" + context + "] ";
}

} // namespace Ragger
