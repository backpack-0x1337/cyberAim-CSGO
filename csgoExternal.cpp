
#include <iostream>
#include <Windows.h>
#include "proc/proc.h"
#include "mem/mem.h"
#include "csgo.hpp"
#include "hwid/hwid.h"
#include "features/bhop.h"


#define FL_ONGROUND (1<<0) // At rest / on the ground

enum Error{
    ERROR_OK,
    ERROR_PROCESS_NOT_FOUND,
    ERROR_HANDLE_INVALID,
    ERROR_MODULEBASEADDRESS_INVALID,
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


/**
 * Load ModuleBaseAddress and process handle into gameData struct
 * @param clientName
 * @param moduleName
 * @param gameData
 * @return ERROR code if any
 */
Error LoadGameModuleAddNHandle(ProcInfo pi, GameData* gameData) {
    DWORD procId = 0;

    procId = GetProcId(pi.clientName);
    if (!procId) {
        return ERROR_PROCESS_NOT_FOUND;
    }

    gameData->hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
    if (!gameData->hProcess) {
        return ERROR_HANDLE_INVALID;
    }

    gameData->clientModuleBaseAddress = GetModuleBaseAddress(procId, pi.clientModuleName);
    if (!gameData->clientModuleBaseAddress) {
        return ERROR_MODULEBASEADDRESS_INVALID;
    }
    gameData->engineModuleBaseAddress = GetModuleBaseAddress(procId, pi.engineModuleName);
    if (!gameData->clientModuleBaseAddress) {
        return ERROR_MODULEBASEADDRESS_INVALID;
    }
    printf( "ProcID: %lu\n", procId);
    printf( "clientModuleBaseAddress: %#010x\n", gameData->clientModuleBaseAddress);
    printf( "engineModuleBaseAddress: %#010x\n", gameData->engineModuleBaseAddress);
    fflush(stdout);
    return ERROR_OK;
}


int BHop(GameData* gd) {
    gd->localPlayer.flags = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hz::netvars::m_fFlags);


    if (GetAsyncKeyState(VK_SPACE) && gd->localPlayer.flags & FL_ONGROUND) {
        int x = 6;
        mem::WPM<int>(gd->hProcess, gd->clientModuleBaseAddress + hz::sig::dwForceJump, x);
    }
    return 0;
}


/**
 * Use CSGO Glow object manager to glow the target ent player
 * @param gd Game Data struct
 * @param glowObject Glow Object manager
 * @param targetEntity The player we want to glow
 * @return
 */
int ActivePlayerGlow(GameData* gd, uintptr_t glowObject, uintptr_t targetEntity) {
    // fix this shitty glow
    int glowIndex = (int)mem::RPM<uintptr_t>(gd->hProcess, targetEntity + hz::netvars::m_iGlowIndex);

    float red = 2;
    mem::WPM<float>(gd->hProcess, (glowObject + ((glowIndex * 0x38) + 0x8)), red);

    float green = 0;
    mem::WPM<float>(gd->hProcess, (glowObject + ((glowIndex * 0x38) + 0xc)), green);

    float blue = 0;
    mem::WPM<float>(gd->hProcess, (glowObject + ((glowIndex * 0x38) + 0x10)), blue);

    float alpha = 1.7;
    mem::WPM<float>(gd->hProcess, (glowObject + ((glowIndex * 0x38) + 0x14)), alpha);

    bool temp = true;
    mem::WPM<bool>(gd->hProcess, (glowObject + ((glowIndex * 0x38) + 0x28)), temp);

    temp = false;
    mem::WPM<bool>(gd->hProcess, (glowObject + ((glowIndex * 0x38) + 0x29)), temp);

    return 0;
}


/**
 * Rewrite csgo ent byte to spoted so they show on players map
 * This function activate the enemy location of minimap
 * @param gd
 * @param targetEntity
 * @return
 */
int ActivateRadarHack(GameData* gd, uintptr_t targetEntity) {
    bool spot = true;
    mem::WPM<bool>(gd->hProcess, targetEntity + hz::netvars::m_bSpotted, spot);
    return 0;
}


/**
 * This function loop through all players ent list
 * @param gd GameData Struct
 * @return 0
 */
int LoopEntList(GameData* gd) {
    // Glow Object Manager
    auto glowObject = mem::RPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress + hz::sig::dwGlowObjectManager);
    for (int i = 0; i < 64; i++) {
        auto targetEntity = mem::RPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress + hz::sig::dwEntityList + i * 0x10);
        if (targetEntity) { // if there is an enemy
            auto targetTeam = mem::RPM<uintptr_t>(gd->hProcess, targetEntity + hz::netvars::m_iTeamNum);
            auto myTeam = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hz::netvars::m_iTeamNum);

            // check whether target is in the enemy team
            if (targetTeam != myTeam && myTeam <= 9) {
                ActivateRadarHack(gd, targetEntity);
                ActivePlayerGlow(gd, glowObject, targetEntity);
            }


        }
    }
    return 0;
}


/**
 * Change the local player flash duration to zero
 * @param gd
 * @return
 */
int AntiFlash(GameData* gd) {
    auto flashDuration = mem::RPM<uintptr_t>(gd->hProcess, (gd->localPlayer.playerEnt + hz::netvars::m_flFlashDuration));
    if (flashDuration > 0) {
        int temp = 0;
        mem::WPM<int>(gd->hProcess, gd->localPlayer.playerEnt + hz::netvars::m_flFlashDuration, temp);
    }
    return 0;
}


/**
 * Cursor travel from original location to destination with given stop amount
 * @param gd
 * @param originalCursorLoc
 * @param targetCursorLoc
 * @param stop
 */
void AimToTargetSmooth(GameData* gd, Vec3 originalCursorLoc, Vec3 targetCursorLoc, int stop) {
    Vec3 diff = (targetCursorLoc - originalCursorLoc) / (float)stop; // 4,0,0

    Vec3 copyOri = originalCursorLoc;
    for (int i = 0; i < stop; ++i) {
        copyOri = copyOri + diff;
        copyOri.Normalize();
        mem::WPM(gd->hProcess, gd->clientState + hz::sig::dwClientState_ViewAngles, copyOri);
    }
}


int checkTBot(GameData* gd) {
    auto crosshair = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hz::netvars::m_iCrosshairId);
    if (crosshair != 0 && crosshair < 64) {
        uintptr_t entity = mem::RPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress + ((crosshair - 1) * 0x10));
        uintptr_t entityTeam = mem::RPM<uintptr_t>(gd->hProcess, entity + hz::netvars::m_iTeamNum);

    }
    return 0;
}


/**
 * Recoil Control System, Counter recoil by moving view angle to the opposite direction of the punch angle
 * @param gd
 * @return
 */
int RCS(GameData* gd) {

    // Get clientState because view angle is in clientState
    gd->clientState = mem::RPM<uintptr_t>(gd->hProcess, gd->engineModuleBaseAddress + hz::sig::dwClientState);
    Vec3 punchAngle = mem::RPM<Vec3>(gd->hProcess, gd->localPlayer.playerEnt + hz::netvars::m_aimPunchAngle);
    Vec3 viewAngle = mem::RPM<Vec3>(gd->hProcess, gd->clientState + hz::sig::dwClientState_ViewAngles);
    int isShotFired = mem::RPM<int>(gd->hProcess, gd->localPlayer.playerEnt + hz::netvars::m_iShotsFired);
    if (isShotFired > 1) {
        Vec3 newViewAngle = ((viewAngle + gd->localPlayer.lastPunch - punchAngle * 2.f));
        newViewAngle.Normalize();
        gd->localPlayer.lastPunch = punchAngle * 2.f;

        AimToTargetSmooth(gd, viewAngle, newViewAngle, 100);
    } else {
        gd->localPlayer.lastPunch = {0,0,0};
    }

    return 0;
}


int MainLogic(GameData* gd) {

    bool active = true;
    DWORD dwExit = 0;
//    gd->localPlayer.lastPunch.x = gd->localPlayer.lastPunch.y = 0;
    while (GetExitCodeProcess(gd->hProcess, &dwExit) && dwExit == STILL_ACTIVE) {

        if (GetAsyncKeyState(VK_OEM_PLUS) & 1) { // key to activate TriggerBot
            std::cout << "Input num plus" << std::endl;
            if (gd->feature.triggerBot) {
                gd->feature.triggerBot = false;
                std::cout << "Trigger Bot" << std::endl;
            }
            continue;
        }
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            // activate cheat or deactivate cheat
            active = !active;
            if (active) {
                printf( "csgo cheat activated\n");
            } else {
                printf( "csgo cheat deactivated\n");
            }
            fflush(stdout);
        }
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }

        // if chair is active
        if (active) {
            gd->localPlayer.playerEnt = mem::RPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress + hz::sig::dwLocalPlayer);
//            checkTBot(gd);
            RCS(gd);
            AntiFlash(gd);
            BHop(gd);
            LoopEntList(gd);
            }
        Sleep(1);
        }
    return ERROR_OK;
}


int main() {

    if (!hwid::isUserHWIDValid()) {
        return 0;
    }

    ProcInfo pi = {"csgo.exe", "client.dll", "engine.dll"};

    Error err;
    GameData gameData{};

    err = LoadGameModuleAddNHandle(pi, &gameData);
    if (err != ERROR_OK) {
        return LogError(err);
    }
    printf( "\n\nWelcome To CyberAim!\n\n");fflush(stdout);
    printf( "Press Insert to Activate!\n\n");fflush(stdout);


    MainLogic(&gameData);
    printf( "Thank you For Choosing CyberAim!\n");fflush(stdout);
    Sleep(1000);
    return 0;
}

