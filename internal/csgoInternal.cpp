
#include <iostream>
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <tchar.h>
#include "../csgo.hpp"

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
    bool radarEsp;
    bool triggerBot;
    bool bHop;
    bool rcs;
    bool antiFlash;

    /**
     * Set Features Flag to False
     */
    void SetupFeatureFlag(bool flag) {
        aimBot  = flag;
        glowEsp = flag;
        triggerBot = flag;
        bHop = flag;
        radarEsp = flag;
        rcs = flag;
        antiFlash = flag;
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

        playerEnt = *(uintptr_t*)(clientModuleBaseAddress + hazedumper::signatures::dwLocalPlayer);

        if (!playerEnt) {
            return ERROR_PLAYERENT_NULL;
        }
        return ERROR_OK;
    }

    Error LoadPlayerFlag() {
        if (!playerEnt) {
            return ERROR_PLAYERENT_NULL;
        }
        flags = (uintptr_t*)(playerEnt + hazedumper::netvars::m_fFlags);
        return ERROR_OK;
    }

    Error LocalPlayerForceJump(uintptr_t clientModuleBaseAddress) {
        fJump = (uintptr_t*)(clientModuleBaseAddress + hazedumper::signatures::dwForceJump);
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

std::vector<uintptr_t> GetEntityList(uintptr_t clientModuleBaseAddress) {
    std::vector<uintptr_t> entityList;
    for (int i = 0; i < 64; ++i) {
        uintptr_t targetEntity = *(uintptr_t*)(clientModuleBaseAddress + hazedumper::signatures::dwEntityList + i * 0x10);
        if (targetEntity) {
            entityList.push_back(targetEntity);
        }
    }
    return entityList;
}


std::vector<uintptr_t> getEnemyEntityList(GameData gd) {

    std::vector<uintptr_t> entityList = GetEntityList(gd.clientModuleBaseAddress);
    std::vector<uintptr_t> enemyEntityList;
    for (int i = 0; i < entityList.size(); ++i) {
        uintptr_t entity = entityList[i];
        int entityTeam = *(int*)(entity + hazedumper::netvars::m_iTeamNum);
        int localPlayerTeam = *(int*)(gd.localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
        if (entityTeam != localPlayerTeam && localPlayerTeam <= 9) {
            enemyEntityList.push_back(entityList[i]);
        }
    }
    return enemyEntityList;
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
    gd->clientState = *(uintptr_t*)(gd->engineModuleBaseAddress + hazedumper::signatures::dwClientState);
    Vec3 punchAngle = *(Vec3*)(gd->localPlayer.playerEnt + hazedumper::netvars::m_aimPunchAngle);
    Vec3* viewAngle= (Vec3*)(gd->clientState + hazedumper::signatures::dwClientState_ViewAngles);
    int shotFired = *(int*)(gd->localPlayer.playerEnt + hazedumper::netvars:: m_iShotsFired);
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

/**
 * Use CSGO Glow object manager to glow the target ent player
 * @param gd Game Data struct
 * @param glowObject Glow Object manager
 * @param targetEntity The player we want to glow
 * @return
 */
void MakePlayerGlow(GameData* gd, uintptr_t targetEntity) {
    uintptr_t glowObject = *(uintptr_t*)(gd->clientModuleBaseAddress + hazedumper::signatures::dwGlowObjectManager);

    int glowIndex = *(int*)(targetEntity + hazedumper::netvars::m_iGlowIndex);

    float red = 2;
    *(float*)(glowObject + ((glowIndex * 0x38) + 0x8)) = red;

    float green = 0;
    *(float*)(glowObject + ((glowIndex * 0x38) + 0xc)) = green;

    float blue = 0;
    *(float*)(glowObject + ((glowIndex * 0x38) + 0x10)) = blue;

    float alpha = 1.7;
    *(float*)(glowObject + ((glowIndex * 0x38) + 0x14)) = alpha;

    bool temp = true;
    *(bool*)(glowObject + ((glowIndex * 0x38) + 0x28)) = temp;

    temp = false;
    *(bool*)(glowObject + ((glowIndex * 0x38) + 0x29)) = temp;

}

void GlowEsp(GameData* gd, std::vector<uintptr_t> enemyEntityList) {
    for (int i = 0; i < enemyEntityList.size(); ++i) {
        MakePlayerGlow(gd, enemyEntityList[i]);
    }
}

/**
 * set target ent to spot so they will show on csgo map
 * @param target the player to modify
 */
void MakeTargetSpotted(uintptr_t target) {
    bool spot = true;
    *(bool*)(target + hazedumper::netvars::m_bSpotted) = spot;
}

void SpotRadarEsp(std::vector<uintptr_t> enemyEntityList) {
    for (int i = 0; i < enemyEntityList.size(); ++i) {
        MakeTargetSpotted(enemyEntityList[i]);
    }
}

void AntiFlash(GameData* gd) {
    int* flashDuration = (int*)(gd->localPlayer.playerEnt + hazedumper::netvars::m_flFlashDuration);
    if (*flashDuration > 0) {
        *flashDuration = 0;
    }
}

void shoot(GameData* gd) {
    *(uintptr_t*)(gd->clientModuleBaseAddress + hazedumper::signatures::dwForceAttack) =  5;
    Sleep(25);
    *(uintptr_t*)(gd->clientModuleBaseAddress + hazedumper::signatures::dwForceAttack) =  4;
    Sleep(25);
}

void TriggerBot(GameData* gd) {
    uintptr_t crossHairTarget = *(uintptr_t*)(gd->localPlayer.playerEnt + hazedumper::netvars::m_iCrosshairId);
    if (!crossHairTarget) {
        uintptr_t crossHairTargetTeam = *(uintptr_t*)(crossHairTarget + hazedumper::netvars::m_iTeamNum);
        uintptr_t crossHairTargetHealth = *(uintptr_t*)(crossHairTarget + hazedumper::netvars::m_iHealth);
        uintptr_t localPlayerTeam = *(uintptr_t*)(gd->localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
        if (crossHairTargetTeam != localPlayerTeam && crossHairTargetHealth != 0) {
            shoot(gd);
        }
    }
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

    err = gd.LoadModuleHandle("client.dll", "engine.dll");
    if (err != ERROR_OK) {LogError(err);}

    gd.feature.SetupFeatureFlag(false); // set all the flags to false

    while (true) {
        // ====================================== Toggle Key ====================================
        if (GetAsyncKeyState(VK_END) & 1) { // first check if end key is pressed
            std::cout << "Thank You For Using Cyber Aim\nExiting...\n" << std::endl;
            break;
        }

        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            if (gd.feature.bHop) {
                gd.feature.bHop = false;
                std::cout << "Toggle BHop OFF" << std::endl;
            } else {
                gd.feature.bHop = true;
                std::cout << "Toggle BHop ON" << std::endl;
            }
        }

        if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
            if (gd.feature.rcs) {
                gd.feature.rcs = false;
                std::cout << "Toggle RCS OFF" << std::endl;
            } else {
                gd.feature.rcs = true;
                std::cout << "Toggle RCS ON" << std::endl;
            }
        }

        if (GetAsyncKeyState(VK_NUMPAD3) & 1) {
            if (gd.feature.glowEsp) {
                gd.feature.glowEsp = false;
                std::cout << "Toggle glowEsp OFF" << std::endl;
            } else {
                gd.feature.glowEsp = true;
                std::cout << "Toggle glowEsp ON" << std::endl;
            }
        }

        if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
            if (gd.feature.radarEsp) {
                gd.feature.radarEsp = false;
                std::cout << "Toggle radarEsp OFF" << std::endl;
            } else {
                gd.feature.radarEsp = true;
                std::cout << "Toggle radarEsp ON" << std::endl;
            }
        }

        if (GetAsyncKeyState(VK_NUMPAD5) & 1) {
            if (gd.feature.antiFlash) {
                gd.feature.antiFlash = false;
                std::cout << "Toggle antiFlash OFF" << std::endl;
            } else {
                gd.feature.antiFlash = true;
                std::cout << "Toggle antiFlash ON" << std::endl;
            }
        }

        if (GetAsyncKeyState(VK_DELETE) & 1) {
            gd.feature.SetupFeatureFlag(false);
            std::cout << "Toggle Everything OFF" << std::endl;
        }

        if (GetAsyncKeyState(VK_INSERT) & 1) {
            gd.feature.SetupFeatureFlag(true);
            std::cout << "Toggle Everything ON" << std::endl;
        }

        // =====================================================================================

        err = gd.localPlayer.LoadLocalPLayerEnt(gd.clientModuleBaseAddress);
        if (err == ERROR_PLAYERENT_NULL) {
            continue;
        }


        if (gd.feature.bHop) {
            gd.localPlayer.LoadPlayerFlag();

            if (GetAsyncKeyState(VK_SPACE) && *gd.localPlayer.flags & FL_ONGROUND) { // BHOP
                gd.localPlayer.LocalPlayerForceJump(gd.clientModuleBaseAddress);
            }
        }

        if (gd.feature.rcs) { // Check if recoil boolean is true
            RCS(&gd);
        }

        if (gd.feature.glowEsp) {
            GlowEsp(&gd, getEnemyEntityList(gd));
        }

        if(gd.feature.radarEsp) {
            SpotRadarEsp(getEnemyEntityList(gd));
        }


        if(gd.feature.antiFlash) {
            AntiFlash(&gd);
        }
//
//        if(gd.feature.triggerBot) {
//            TriggerBot(&gd);
//        }



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