#include "heUtils.h"

std::vector<std::string> heStringSplit(const std::string& string, const char delimn) {
    
    std::vector<std::string> result;
    std::string::const_iterator start = string.begin();
    std::string::const_iterator end = string.end();
    std::string::const_iterator next = std::find(start, end, delimn);
    
    while (next != end) {
        result.emplace_back(std::string(start, next));
        start = next + 1;
        
        while(start != end && start[0] == delimn)
            ++start;
        
        next = std::find(start, end, delimn);
        
    }
    
    if(start != end)
        result.emplace_back(std::string(start, next));
    return result;
    
};

std::string heStringReplaceAll(const std::string& input, const std::string& from, const std::string& to) {
    
    std::string copy = input;
    size_t start_pos = 0;
    while((start_pos = copy.find(from, start_pos)) != std::string::npos) {
        copy.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    
    return input;
    
}; 

bool heStringStartsWith(const std::string& base, const std::string& check) {
    
    return base.compare(0, check.size(), check.c_str()) == 0;
    
};