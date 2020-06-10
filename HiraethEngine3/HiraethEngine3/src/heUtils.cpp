#include "hepch.h"
#include "heUtils.h"

std::string SPACE_CHARS = "\t\n\v\f\r";


void heRandomCreate(HeRandom* random, uint32_t const seed) {
    if(seed > 0)
        random->gen = std::mt19937(seed);
    else {
        std::random_device device;
        random->gen = std::mt19937(device());    
    }
};

int32_t heRandomInt(HeRandom* random, int32_t const low, int32_t const high) {
    random->idis = std::uniform_int_distribution<>(low, high);
    return random->idis(random->gen);
};

float heRandomFloat(HeRandom* random, float const low, float const high) {
    random->fdis = std::uniform_real_distribution<float>(low, high);
    return random->fdis(random->gen);
};


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
    
    return copy;
};

std::string heStringReplaceAll(const std::string& input, char const from, const char to) {
    std::string copy = input;
    size_t start_pos = 0;
    while((start_pos = copy.find(from, start_pos)) != std::string::npos) {
        //copy.replace(start_pos, 1, to);
        copy[start_pos] = to;
        start_pos += 1; // Handles case where 'to' is a substring of 'from'
    }
    
    return copy;
};

b8 heStringStartsWith(const std::string& base, const std::string& check) {
    return base.compare(0, check.size(), check.c_str()) == 0;
};

void heStringEatSpacesLeft(std::string& string) {
    string.erase(0, string.find_first_not_of(SPACE_CHARS));
};

void heStringEatSpacesRight(std::string& string) {
    string.erase(string.find_last_not_of(SPACE_CHARS) + 1);
};
