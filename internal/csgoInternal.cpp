
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <tchar.h>
#include "../csgo.hpp"
#include "../mem/mem.h"

#define FL_ONGROUND (1<<0) // At rest / on the ground

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
    bool aimBot;
    bool glowEsp;
    bool triggerBot;
    bool bHop;
    bool rcs;

    /**
     * Set Features Flag to False
     */
    void SetupFeatureFlag() {
        aimBot  = false;
        glowEsp = false;
        triggerBot = false;
        bHop = false;
        rcs = false;
    }
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

// loop throught entity list and find all enemy object
// Put it in a array and return it
uintptr_t* getEnemyEntityList() {
    return nullptr;
}

/**
 * Cursor travel from original location to destination with given stop amount
 * @param gd
 * @param originalCursorLoc
 * @param targetCursorLoc
 * @param stop
 */
void AimToTargetSmooth(Vec3* pViewAngle, Vec3 originalCursorLoc, Vec3 targetCursorLoc, int stop) {
    Vec3 diff = (targetCursorLoc - originalCursorLoc) / (float)stop; // 4,0,0

    Vec3 copyOri = originalCursorLoc;
    for (int i = 0; i < stop; ++i) {
        copyOri = copyOri + diff;
        copyOri.Normalize();
        std::cout << copyOri.x << std::endl; // debug
        *pViewAngle = copyOri;
    }
}

/**
 * AutoMatic control gun recoil by reversing view angle by the amount of punch angle * 2
 * @param gd
 */
void RCS(GameData* gd) {
    gd->clientState = *(uintptr_t*)(gd->engineModuleBaseAddress + hz::sig::dwClientState);
    Vec3 punchAngle = *(Vec3*)(gd->localPlayer.playerEnt + hz::netvars::m_aimPunchAngle);
    Vec3* viewAngle= (Vec3*)(gd->clientState + hz::sig::dwClientState_ViewAngles);
    int shotFired = *(int*)(gd->localPlayer.playerEnt + hz::netvars:: m_iShotsFired);
    if (shotFired > 1) {
        Vec3 newViewAngle = *viewAngle + gd->localPlayer.lastPunch - punchAngle*2.f;
        newViewAngle.Normalize();
        gd->localPlayer.lastPunch = punchAngle * 2.f;

        AimToTargetSmooth(viewAngle, *viewAngle, newViewAngle, 150);
    } else {
        gd->localPlayer.lastPunch = {0,0,0};
    }
    return;
}

void WINAPI HackThread(HMODULE hModule) {

    //Create Console
    AllocConsole();
    FILE *f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "\n\nWelcome To CyberAim!\n\n" << std::endl;
    std::cout << "Press Insert to Activate!\n\n" << std::endl;


    GameData gd;
    Error err;
    gd.feature.SetupFeatureFlag();

    err = gd.LoadModuleHandle("client.dll", "engine.dll");
    if (err != ERROR_OK) {LogError(err);}

    gd.feature.SetupFeatureFlag(); // set all the flags to false

    while (true) {

        // ====================================== Toggle Key ====================================
        if (GetAsyncKeyState(VK_END) & 1) { // first check if end key is pressed
            std::cout << "Thank You For Using Cyber Aim\nExiting...\n" << std::endl;
            break;
        }

        if (GetAsyncKeyState(VK_NUMPAD7) & 1) {
            if (gd.feature.bHop) {
                gd.feature.bHop = false;
                std::cout << "Toggle BHop OFF" << std::endl;
            } else {
                gd.feature.bHop = true;
                std::cout << "Toggle BHop ON" << std::endl;
            }
        }

        if (GetAsyncKeyState(VK_NUMPAD8) & 1) {
            if (gd.feature.rcs) {
                gd.feature.rcs = false;
                std::cout << "Toggle RCS OFF" << std::endl;
            } else {
                gd.feature.rcs = true;
                std::cout << "Toggle RCS ON" << std::endl;
            }
        }


        // =====================================================================================

        gd.localPlayer.LoadLocalPLayerEnt(gd.clientModuleBaseAddress);

        if (gd.feature.bHop) {
            gd.localPlayer.LoadPlayerFlag();

            if (GetAsyncKeyState(VK_SPACE) && *gd.localPlayer.flags & FL_ONGROUND) { // BHOP
                gd.localPlayer.LocalPlayerForceJump(gd.clientModuleBaseAddress);
            }
        }

        if (gd.feature.rcs) { // Check if recoil boolean is true
            RCS(&gd);
        }


        Sleep(1);
    }
    Sleep(500);
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