#include "heCore.h"
#include <ctime>
#include <iostream>

HeDebugInfo heDebugInfo;

void heLogCout(const std::string& message, const std::string& prefix) {
    
    // get time
    auto time = std::time(nullptr);
    struct tm buf;
    localtime_s(&buf, &time);
    std::string timeString;
    timeString.reserve(10);
    std::strftime(timeString.data(), 10, "[%H:%M:%S]", &buf);
    
    std::string output;
    output.reserve(19 + message.size());
    output.append(timeString);
    output.append(prefix);
    output.append(message);
    output.push_back('\n');
    std::cout << output;
    std::cout.flush();
    
};

void heDebugRequestInfo(const HeDebugInfoFlags flags) {
    
    heDebugInfo.flags |= flags;
    
};

b8 heDebugIsInfoRequested(const HeDebugInfoFlags flag) {
    
    b8 set = heDebugInfo.flags & flag;
    heDebugInfo.flags = heDebugInfo.flags & ~flag;
    return set;
    
};

void heDebugSetOutput(std::ostream* stream) {
    
    heDebugInfo.stream = stream;
    
};

void heDebugPrint(const std::string& message) {
    
    // get time
    auto time = std::time(nullptr);
    struct tm buf;
    localtime_s(&buf, &time);
    std::string timeString;
    timeString.reserve(10);
    std::strftime(timeString.data(), 10, "[%H:%M:%S]", &buf);
    
    std::string output;
    output.reserve(19 + message.size());
    output.append(timeString);
    output.append("[DEBUG]:");
    output.append(message);
    output.push_back('\n');
    
    if (heDebugInfo.stream == nullptr) {
        std::cout << output;
        std::cout.flush();
    } else {
        *heDebugInfo.stream << output;
        heDebugInfo.stream->flush();
    }
    
};