
#include "csgoInternal.h"
#include "Windows.h"
#include "stdio.h"

struct Player {
    uintptr_t playerEnt;
    uintptr_t fJump;
    DWORD flags;
    uintptr_t crossHair;
    Vec3 lastPunch = {0,0,0};
};

struct Feature {
    bool aimBot = false;
    bool glowEsp = true;
    bool triggerBot = false;
    bool bHop = true;
    bool rcs = true;
};

struct GameData {
    HANDLE hProcess;
    uintptr_t clientModuleBaseAddress;
    uintptr_t engineModuleBaseAddress;
    Player localPlayer;
    uintptr_t clientState;
    Feature feature;
};

struct ProcInfo {
    LPCTSTR clientName;
    LPCTSTR clientModuleName;
    LPCTSTR engineModuleName;
};

void Main() {
    GameData gd;

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hModule);
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Main, NULL, NULL,NULL)
        }
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}