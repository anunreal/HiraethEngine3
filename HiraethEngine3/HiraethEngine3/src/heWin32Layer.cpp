#include "heWin32Layer.h"
#include "heCore.h"
#include <map>
#include <iostream>
#include <windows.h>

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
        HANDLE handle = CreateFile(std::wstring(file.begin(), file.end()).c_str(), 
                                   GENERIC_READ, 
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        
        if(handle == INVALID_HANDLE_VALUE)
            HE_ERROR("Could not open file handle for time checking (" + file + ")");
        else {
            handleMap[file] = handle;
            FILETIME time;
            GetFileTime(handle, nullptr, nullptr, &time);
            timeMap[file] = time;
        }
        
        return false;
    } else {
        
        FILETIME time;
        GetFileTime(handleMap[file], nullptr, nullptr, &time);
        const FILETIME& last = timeMap[file];
        bool wasModified = (last.dwLowDateTime != time.dwLowDateTime || last.dwHighDateTime != time.dwHighDateTime);
        timeMap[file] = time;
        return wasModified;
        
    }
    
};