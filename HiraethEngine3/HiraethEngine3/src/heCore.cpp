#include "heCore.h"
#include "heConsole.h"
#include <ctime>
#include <iostream>
#include <vector>

void heLogCout(std::string const& message, std::string const& prefix) {
    
    // get time
    auto time = std::time(nullptr);
    struct tm buf;
    localtime_s(&buf, &time);
    char timeBuf[11];
    std::strftime(timeBuf, 11, "[%H:%M:%S]", &buf);
    std::string timeString(timeBuf, 10);
    
    std::string output;
    output.reserve(20 + message.size());
    output.append(timeString);
    output.append(prefix);
    output.push_back(' ');
    output.append(message);

	heConsolePrint(output);

	output.push_back('\n');
    std::cout << output;
    std::cout.flush();
    
};
