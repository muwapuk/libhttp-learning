#include "logger.h"
#include <fmt/core.h>
#include <fmt/chrono.h>

namespace Logger {
    Logger g_Logger{};
    
    Logger::Logger(std::ostream& out)
    {
        this->out = &out;
    }

void Logger::setOutStream(std::ostream& out)
{
    this->out = &out;
}

void Logger::logMessage(LogLevel level, const std::string& message, const char *fileName, int lineNum)
{
    if (level < g_LogLevel) return;
    auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
    std::string currentTime { fmt::format("{:%H:%M:%S}", now) };
    *this->out << levels[level] << ": " << currentTime 
               //<< " [" << fileName << '|' << lineNum << "]:" 
               << ": " << message << std::endl; 
}

}
