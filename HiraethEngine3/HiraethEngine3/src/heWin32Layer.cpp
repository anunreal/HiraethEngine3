#include "heWin32Layer.cpp"

// move this shit into cpp prolly and then how to store last modification time?
typedef std::map<std::string, FILETIME> HeFileTimeMap;
// stores a file handle to any possible file in this map for faster lookups
typedef std::map<std::string, HANDLE> HeFileHandleMap;

HeFileTimeMap timeMap;
HeFileHandleMap handleMap;



bool heFileModified(const std::string& file) {
    
    auto it = handleMap.find(file);
    if(it == handleMap.end()) {
        // file was never requested before, load now
        HANDLE handle = heCreateFile(file.c_str(), 
                                     GENERIC_READ, 
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
        
        if(handle == INVALID_HANDLE)
            std::cout << "Error: Could not open file handle for time checking (" << file << ")" << std::endl;
        else {
            handleMap[file] = handle;
            FILETIME time;
            GetFileTimeA(handle, nullptr, nullptr, time);
            timeMap[file] = time;
        }
        
        return false;
    } else {
        
        FILETIME time;
        GetFileTimeA(handleMap[file], nullptr, nullptr, time);
        bool wasModified = timeMap[file] == time;
        timeMap[file] = time;
        return wasModified;
        
    }
    
};