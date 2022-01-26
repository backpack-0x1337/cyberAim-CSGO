
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <tchar.h>
#include "../csgo.hpp"

enum Error{
    ERROR_OK,
    ERROR_PROCESS_NOT_FOUND,
    ERROR_HANDLE_INVALID,
    ERROR_MODULEBASEADDRESS_INVALID,
    ERROR_CLIENTMOD_INVALID,
    ERROR_ENGINEMOD_INVALID,
    ERROR_PLAYERENT_NULL,


};

Error LogError (Error err) {
    if (err == ERROR_PROCESS_NOT_FOUND) {
        std::cerr << "Process ID Not Found\n";
    }
    if (err == ERROR_HANDLE_INVALID) {
        std::cerr << "ERROR_HANDLE_INVALID\n";
    }
    if (err == ERROR_MODULEBASEADDRESS_INVALID) {
        std::cerr << "ERROR_MODULEBASEADDRESS_INVALID\n";
    }
    if (err == ERROR_CLIENTMOD_INVALID) {
        std::cerr << "ERROR_CLIENTMOD_INVALID\n";
    }
    if (err == ERROR_MODULEBASEADDRESS_INVALID) {
        std::cerr << "ERROR_MODULEBASEADDRESS_INVALID\n";
    }
    if (err == ERROR_PLAYERENT_NULL) {
        std::cerr << "ERROR_PLAYERENT_NULL\n";
    }
    getchar();
    return err;
}


struct Vec3{
    float x,y,z;

    Vec3 operator-(Vec3 d) {
        return { x - d.x, y - d.y, z - d.z };
    }
    Vec3 operator+(Vec3 d) {
        return { x + d.x, y + d.y, z + d.z };
    }
    Vec3 operator*(float d) {
        return { x * d, y * d, z * d };
    }

    Vec3 operator/(float d) {
        return { x / d, y / d, z / d };
    }
    void Normalize() {
        while (y < -180) { y += 360; }
        while (y > 180) { y -= 360; }
        if (x > 89) { x = 89; }
        if (x < -89) { x = -89; }
    }
};

struct Feature {
    bool aimBot = false;
    bool glowEsp = true;
    bool triggerBot = false;
    bool bHop = true;
    bool rcs = true;
};


struct Player {
    uintptr_t playerEnt;
    uintptr_t* fJump;
    uintptr_t* flags;
    uintptr_t crossHair;
    Vec3 lastPunch = {0,0,0};

    Error LoadLocalPLayerEnt(uintptr_t clientModuleBaseAddress) {
        if (!clientModuleBaseAddress) {
            return ERROR_CLIENTMOD_INVALID;
        }

        playerEnt = *(uintptr_t*)(clientModuleBaseAddress + hz::sig::dwLocalPlayer);

        if (!playerEnt) {
            return ERROR_PLAYERENT_NULL;
        }
        return ERROR_OK;
    }

    Error LoadPlayerFlag() {
        if (!playerEnt) {
            return ERROR_PLAYERENT_NULL;
        }
        flags = (uintptr_t*)(playerEnt + hz::netvars::m_fFlags);
        return ERROR_OK;
    }

    Error LocalPlayerForceJump(uintptr_t clientModuleBaseAddress) {
        fJump = (uintptr_t*)(clientModuleBaseAddress + hz::sig::dwForceJump);
        *fJump = 6;
        return ERROR_OK;
    }
};


struct GameData {
//    HANDLE hProcess;
    uintptr_t clientModuleBaseAddress;
    uintptr_t engineModuleBaseAddress;
    Player localPlayer;
    uintptr_t clientState;
    Feature feature;

    Error LoadModuleHandle(LPCTSTR clientModuleName, LPCTSTR engineModuleName) {
        this->clientModuleBaseAddress = (uintptr_t)GetModuleHandle("client.dll");
        this->engineModuleBaseAddress = (uintptr_t)GetModuleHandle("engine.dll");
        if (!this->clientModuleBaseAddress || !this->engineModuleBaseAddress) {
            return ERROR_MODULEBASEADDRESS_INVALID;
        }
        return ERROR_OK;
    }
};

struct ProcInfo {
    LPCTSTR clientName;
    LPCTSTR clientModuleName;
    LPCTSTR engineModuleName;
};

DWORD WINAPI HackThread(HMODULE hModule) {

    //Create Console
    AllocConsole();
    FILE *f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "\n\nWelcome To CyberAim!\n\n" << std::endl;
    std::cout << "\"Press Insert to Activate!\nn\n" << std::endl;


    GameData gd;
    Error err;

    err = gd.LoadModuleHandle("client.dll", "engine.dll");
    if (err != ERROR_OK) {LogError(err);}


    while (true) {

        if (GetAsyncKeyState(VK_END) & 1) { // first check if end key is pressed
            std::cout << "exiting" << std::endl;
            break;
        }

        if (!gd.localPlayer.playerEnt) { // if player ent is null
            gd.localPlayer.LoadLocalPLayerEnt(gd.clientModuleBaseAddress);
            continue; //back to the start of the loop
        }
        err = gd.localPlayer.LoadPlayerFlag();

         if (GetAsyncKeyState(VK_SPACE) && *gd.localPlayer.flags & (1 << 0)) {
            gd.localPlayer.LocalPlayerForceJump(gd.clientModuleBaseAddress);
        }
        Sleep(1);
    }
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall)
    {
        case DLL_PROCESS_ATTACH:
        {
            CloseHandle(CreateThread(nullptr,
                                     0,
                                     (LPTHREAD_START_ROUTINE) HackThread,
                                     hModule,
                                     0,
                                     nullptr));
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}