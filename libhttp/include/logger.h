#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <mutex>

namespace Logger {

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
};

#ifdef GLOBAL_LOG_LEVEL
    const LogLevel g_LogLevel = GLOBAL_LOG_LEVEL;
#else
    const LogLevel g_LogLevel = DEBUG;
#endif

#define logDebug(message) Logger::g_Logger.logMessage(Logger::DEBUG, message, __FILE__, __LINE__)
#define logInfo(message) Logger::g_Logger.logMessage(Logger::INFO, message, __FILE__, __LINE__)
#define logWarn(message) Logger::g_Logger.logMessage(Logger::WARN, message, __FILE__, __LINE__)
#define logError(message) Logger::g_Logger.logMessage(Logger::ERROR, message, __FILE__, __LINE__)

const std::string levels[] {"DEBUG", "INFO", "WARN", "ERROR"};

class Logger {
public:
    explicit Logger(std::ostream& out = std::cout);

    void setOutStream(std::ostream& out);
    void logMessage(LogLevel level, const std::string& message, const char* fileName, int lineNum);
private:
    std::mutex log_mutex;
    std::ostream *out;
};
extern Logger g_Logger;
}

#endif
