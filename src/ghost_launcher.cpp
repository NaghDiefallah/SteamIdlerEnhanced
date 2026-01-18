#include <windows.h>
#include <tlhelp32.h>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>
#include <ctime>

typedef bool (*SteamAPI_Init_t)();
typedef void (*SteamAPI_Shutdown_t)();
typedef bool (*SteamAPI_RestartAppIfNecessary_t)(uint32_t);

// Function to write errors to a file with timestamp
void LogError(const std::string& msg) {
    std::ofstream log("launcher_error.log", std::ios::app);
    if (log.is_open()) {
        std::time_t now = std::time(nullptr);
        char timebuf[64];
        ctime_s(timebuf, sizeof(timebuf), &now);
        log << "[" << timebuf;
        log.seekp(-1, std::ios_base::end);  // Remove newline from ctime_s
        log << "] " << msg << " (Error Code: " << GetLastError() << ")" << std::endl;
    }
}

void LogInfo(const std::string& msg) {
    std::ofstream log("launcher_error.log", std::ios::app);
    if (log.is_open()) {
        std::time_t now = std::time(nullptr);
        char timebuf[64];
        ctime_s(timebuf, sizeof(timebuf), &now);
        log << "[" << timebuf;
        log.seekp(-1, std::ios_base::end);
        log << "] " << msg << std::endl;
    }
}

DWORD GetParentProcessId() {
    DWORD ppid = 0;
    DWORD pid = GetCurrentProcessId();
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LogError("Failed to create process snapshot");
        return 0;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                ppid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return ppid;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    LogInfo("Ghost launcher started");
    
    // Verify steam_appid.txt exists and is readable
    std::ifstream file("steam_appid.txt");
    if (!file.is_open()) {
        LogError("Failed to open steam_appid.txt in directory: " + std::string(GetCommandLineA()));
        return 1;
    }
    
    std::string appIdStr;
    std::getline(file, appIdStr);
    file.close();
    
    // Trim whitespace
    appIdStr.erase(0, appIdStr.find_first_not_of(" \n\r\t"));
    appIdStr.erase(appIdStr.find_last_not_of(" \n\r\t") + 1);
    
    if (appIdStr.empty()) {
        LogError("AppID file was empty or invalid");
        return 1;
    }
    
    uint32_t appId = 0;
    try {
        appId = std::stoul(appIdStr);
    } catch (...) {
        LogError("Failed to parse AppID: " + appIdStr);
        return 1;
    }

    LogInfo("Loaded AppID: " + appIdStr);

    // Set Environment Variables
    if (!SetEnvironmentVariableA("SteamAppId", appIdStr.c_str())) {
        LogError("Failed to set SteamAppId environment variable");
    }
    if (!SetEnvironmentVariableA("SteamGameId", appIdStr.c_str())) {
        LogError("Failed to set SteamGameId environment variable");
    }

    // Load Steam DLL
    HMODULE steamDLL = LoadLibraryA("steam_api64.dll");
    if (!steamDLL) {
        LogError("Could not load steam_api64.dll - ensure it exists in the working directory");
        return 2;
    }

    LogInfo("Loaded steam_api64.dll successfully");

    auto RestartIfReq = (SteamAPI_RestartAppIfNecessary_t)GetProcAddress(steamDLL, "SteamAPI_RestartAppIfNecessary");
    auto SteamAPI_Init = (SteamAPI_Init_t)GetProcAddress(steamDLL, "SteamAPI_Init");
    auto SteamAPI_Shutdown = (SteamAPI_Shutdown_t)GetProcAddress(steamDLL, "SteamAPI_Shutdown");

    if (!SteamAPI_Init || !SteamAPI_Shutdown) {
        LogError("Failed to get SteamAPI function pointers");
        FreeLibrary(steamDLL);
        return 3;
    }

    // CRITICAL: Tells Steam "I am this game"
    if (RestartIfReq && RestartIfReq(appId)) {
        LogInfo("Steam is restarting this app via its own client");
        FreeLibrary(steamDLL);
        return 0;
    }

    if (SteamAPI_Init()) {
        LogInfo("SteamAPI_Init() successful");
        
        DWORD parentPid = GetParentProcessId();
        HANDLE hParent = NULL;
        
        if (parentPid != 0) {
            hParent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
            
            if (hParent != NULL) {
                LogInfo("Successfully opened parent process handle");
                // Wait until the Main GUI closes
                DWORD waitResult = WaitForSingleObject(hParent, INFINITE);
                if (waitResult == WAIT_OBJECT_0) {
                    LogInfo("Parent process terminated, shutting down");
                } else {
                    LogError("WaitForSingleObject returned unexpected result");
                }
                CloseHandle(hParent);
            } else {
                LogError("Failed to open parent process handle - will run for safety period");
                Sleep(30000);  // Run for 30 seconds if parent not found
            }
        } else {
            LogError("Could not get parent process ID - will run for safety period");
            Sleep(30000);
        }
        
        if (SteamAPI_Shutdown) {
            SteamAPI_Shutdown();
            LogInfo("SteamAPI_Shutdown() complete");
        }
    } else {
        LogError("SteamAPI_Init() failed - is Steam running? Check Steam is installed and running");
        Sleep(5000);
    }

    FreeLibrary(steamDLL);
    LogInfo("Ghost launcher exiting normally");
    return 0;
}