#pragma once

#include <exception>
#include <string>
#include "ragger_plugin_api.h"

namespace Ragger {

/**
 * Base exception class for all RAGger exceptions
 */
class RaggerException : public std::exception {
private:
    std::string message_;
    RaggerErrorCode errorCode_;

public:
    explicit RaggerException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN)
        : message_(message), errorCode_(code) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    RaggerErrorCode getErrorCode() const noexcept {
        return errorCode_;
    }

    std::string getMessage() const noexcept {
        return message_;
    }
};

/**
 * EventBus related exceptions
 */
class EventBusException : public RaggerException {
public:
    explicit EventBusException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN)
        : RaggerException("EventBus: " + message, code) {}
};

/**
 * IndexManager related exceptions
 */
class IndexManagerException : public RaggerException {
public:
    explicit IndexManagerException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN)
        : RaggerException("IndexManager: " + message, code) {}
};

/**
 * ContextEngine related exceptions
 */
class ContextEngineException : public RaggerException {
public:
    explicit ContextEngineException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN)
        : RaggerException("ContextEngine: " + message, code) {}
};

/**
 * PluginManager related exceptions
 */
class PluginManagerException : public RaggerException {
public:
    explicit PluginManagerException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN)
        : RaggerException("PluginManager: " + message, code) {}
};

/**
 * Configuration related exceptions
 */
class ConfigException : public RaggerException {
public:
    explicit ConfigException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN)
        : RaggerException("Config: " + message, code) {}
};

/**
 * Database related exceptions
 */
class DatabaseException : public RaggerException {
public:
    explicit DatabaseException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_DATABASE_ERROR)
        : RaggerException("Database: " + message, code) {}
};

/**
 * File system related exceptions
 */
class FileSystemException : public RaggerException {
public:
    explicit FileSystemException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_FILE_NOT_FOUND)
        : RaggerException("FileSystem: " + message, code) {}
};

/**
 * Network related exceptions
 */
class NetworkException : public RaggerException {
public:
    explicit NetworkException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_NETWORK_ERROR)
        : RaggerException("Network: " + message, code) {}
};

/**
 * Threading related exceptions
 */
class ThreadingException : public RaggerException {
public:
    explicit ThreadingException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_THREAD_CREATION)
        : RaggerException("Threading: " + message, code) {}
};

/**
 * Memory related exceptions
 */
class MemoryException : public RaggerException {
public:
    explicit MemoryException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_MEMORY_ALLOCATION)
        : RaggerException("Memory: " + message, code) {}
};

/**
 * Timeout related exceptions
 */
class TimeoutException : public RaggerException {
public:
    explicit TimeoutException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_TIMEOUT)
        : RaggerException("Timeout: " + message, code) {}
};

/**
 * Validation related exceptions
 */
class ValidationException : public RaggerException {
public:
    explicit ValidationException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_INVALID_ARGUMENT)
        : RaggerException("Validation: " + message, code) {}
};

} // namespace Ragger
