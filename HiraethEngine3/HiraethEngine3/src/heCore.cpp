#include "heCore.h"
#include "heUtils.h"
#include "heD3.h"
#include "heDebugUtils.h"
#include <ctime>
#include <iostream>
#include <vector>

HeDebugInfo heDebugInfo;

void heLogCout(const std::string& message, const std::string& prefix) {
    
    // get time
    auto time = std::time(nullptr);
    struct tm buf;
    localtime_s(&buf, &time);
    char timeBuf[11];
    std::strftime(timeBuf, 11, "[%H:%M:%S]", &buf);
    std::string timeString(timeBuf, 10);
    
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
    
    //heDebugInfo.flags |= flags;
    // print debug information
    
    if(flags == HE_DEBUG_INFO_LIGHTS) {
        HeD3Level* level = heD3LevelGetActive();
        heDebugPrint("=== LIGHTS === ");
        for(const auto& all : level->lights) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint(string);
        }
        
        heDebugPrint("=== LIGHTS ===");
    }
    
    if(flags == HE_DEBUG_INFO_INSTANCES) {
        HeD3Level* level = heD3LevelGetActive();
        heDebugPrint("=== INSTANCES ===");
        for(const auto& all : level->instances) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint(string);
        }
        
        heDebugPrint("=== INSTANCES ===");
    }
    
    if(flags & HE_DEBUG_INFO_CAMERA) {
        HeD3Level* level = heD3LevelGetActive();
        heDebugPrint("=== CAMERA ===");
        std::string string;
        he_to_string(&level->camera, string);
        heDebugPrint(string);
        heDebugPrint("=== CAMERA ===");
    }
    
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
    
    std::string output;
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

