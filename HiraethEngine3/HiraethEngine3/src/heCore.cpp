#include "heCore.h"
#include <ctime>
#include <iostream>

void hnLogCout(const std::string& message, const std::string& prefix) {
    
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