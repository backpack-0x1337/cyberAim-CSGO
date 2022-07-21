
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "proc/proc.h"
#include "mem/mem.h"
#include "csgo.hpp"
#include "hwid/hwid.h"
#include "features/bhop.h"
#include "menu.h"


#define FL_ONGROUND (1<<0) // At rest / on the ground
#define EnemyPen 0x000000FF


enum Error {
    ERROR_OK,
    ERROR_PROCESS_NOT_FOUND,
    ERROR_HANDLE_INVALID,
    ERROR_MODULEBASEADDRESS_INVALID,
};

Error LogError(Error err) {
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

struct Vec3 {
    float x, y, z;

    Vec3 operator-(Vec3 d) {
        return {x - d.x, y - d.y, z - d.z};
    }

    Vec3 operator+(Vec3 d) {
        return {x + d.x, y + d.y, z + d.z};
    }

    Vec3 operator*(float d) {
        return {x * d, y * d, z * d};
    }

    Vec3 operator/(float d) {
        return {x / d, y / d, z / d};
    }

    void Normalize() {
        while (y < -180) { y += 360; }
        while (y > 180) { y -= 360; }
        if (x > 89) { x = 89; }
        if (x < -89) { x = -89; }
    }
};

struct ViewMatrix {
    float *operator[](int index) {
        return matrix[index];
    }

    float matrix[4][4];
};

struct Player {
    uintptr_t playerEnt;
    uintptr_t fJump;
    DWORD flags;
    uintptr_t crossHair;
    Vec3 lastPunch = {0, 0, 0};
};


struct BoxEspConfig {
    HBRUSH enemyBrush;
    int screenX;
    int screenY;

    void SetEnemyBrushColor() {
        enemyBrush = CreateSolidBrush(0x000000FF);
    }

    void SetScreenSize(int size_x, int size_y) {
        screenX = 2560;
        screenY = 1440;
    }

    /**
     *
     *
     * Set Esp Config Color and set size of screen
     */
    void SetDefaultConfig() {
        SetEnemyBrushColor();
        SetScreenSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }
};

struct GameData {
    HANDLE hProcess;
    uintptr_t clientModuleBaseAddress;
    uintptr_t engineModuleBaseAddress;
    Player localPlayer;
    Player playerList[64];
    uintptr_t clientState;
    BoxEspConfig boxEspData;
    HDC hdc;
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
Error LoadGameModuleAddNHandle(ProcInfo pi, GameData *gameData) {
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
    printf("ProcID: %lu\n", procId);
    printf("clientModuleBaseAddress: %#010x\n", gameData->clientModuleBaseAddress);
    printf("engineModuleBaseAddress: %#010x\n", gameData->engineModuleBaseAddress);
    fflush(stdout);
    return ERROR_OK;
}


int BHop(GameData *gd) {
    gd->localPlayer.flags = mem::RPM<uintptr_t>(gd->hProcess,
                                                gd->localPlayer.playerEnt + hazedumper::netvars::m_fFlags);

    if (GetAsyncKeyState(VK_SPACE) && gd->localPlayer.flags & FL_ONGROUND) {
        int x = 6;
        printf("jump");
        mem::WPM<int>(gd->hProcess, gd->clientModuleBaseAddress + hazedumper::signatures::dwForceJump, x);
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
int makeEntGlow(GameData *gd, uintptr_t targetEntity) {
    // fix this shitty glow
    auto glowObject = mem::RPM<uintptr_t>(gd->hProcess,
                                          gd->clientModuleBaseAddress + hazedumper::signatures::dwGlowObjectManager);
    int glowIndex = (int) mem::RPM<uintptr_t>(gd->hProcess, targetEntity + hazedumper::netvars::m_iGlowIndex);

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
bool isEntValidEnemy(GameData* gd, uintptr_t pEnt, uintptr_t myTeam) {
    if (!pEnt) {
        return false;
    }
    auto entityHealth = mem::RPM<uintptr_t>(gd->hProcess, pEnt + hazedumper::netvars::m_iHealth);
    auto pEntTeam = mem::RPM<uintptr_t>(gd->hProcess, pEnt + hazedumper::netvars::m_iTeamNum);
    if (entityHealth <= 0 || entityHealth > 100) {
        return false;
    }
    if (pEntTeam != myTeam && myTeam <= 9) {
        return true;
    }
    return false;
}
void GlowEsp(GameData *gd) {
    auto myTeam = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
    auto playerList = gd->playerList;
    for (int i = 0; i < 64; ++i) {
        auto pEnt = playerList[i].playerEnt;
        if (!isEntValidEnemy(gd, pEnt, myTeam)) {continue;}
        makeEntGlow(gd, pEnt);
    }
}
/**
 * Rewrite csgo ent byte to spoted so they show on players map
 * This function activate the enemy location of minimap
 * @param gd
 * @param targetEntity
 * @return
 */
void RadarEsp(GameData *gd) {
    auto myTeam = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
    auto playerList = gd->playerList;
    for (int i = 0; i < 64; ++i) {
        auto pEnt = playerList[i].playerEnt;
        if (!isEntValidEnemy(gd, pEnt, myTeam)) {continue;}
        bool spot = true;
        mem::WPM<bool>(gd->hProcess, pEnt + hazedumper::netvars::m_bSpotted, spot);
    }
}


/**
 * Change the local player flash duration to zero
 * @param gd
 * @return
 */
int AntiFlash(GameData *gd) {
    auto flashDuration = mem::RPM<uintptr_t>(gd->hProcess,
                                             (gd->localPlayer.playerEnt + hazedumper::netvars::m_flFlashDuration));
    if (flashDuration > 0) {
        int temp = 0;
        mem::WPM<int>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_flFlashDuration, temp);
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
void AimToTargetSmooth(GameData *gd, Vec3 originalCursorLoc, Vec3 targetCursorLoc, int stop) {
    Vec3 diff = (targetCursorLoc - originalCursorLoc) / (float) stop; // 4,0,0

    Vec3 copyOri = originalCursorLoc;
    for (int i = 0; i < stop; ++i) {
        copyOri = copyOri + diff;
        copyOri.Normalize();
        mem::WPM(gd->hProcess, gd->clientState + hazedumper::signatures::dwClientState_ViewAngles, copyOri);
    }
}


/**
 * Set CSGO dwforceattack to 5 represent shooot and then 4 to unshoot
 * @param gd
 */
void shoot(GameData *gd) {

    mem::WPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress + hazedumper::signatures::dwForceAttack, 5);
    Sleep(25);
    mem::WPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress + hazedumper::signatures::dwForceAttack, 4);
    Sleep(25);
}


/**
 * check if crosshair is aiming at enemy
 * @param gd
 * @return
 */
int checkTBot(GameData *gd) {
    auto crosshair = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iCrosshairId);

    if (crosshair != 0 && crosshair < 64) {
        auto entity = mem::RPM<uintptr_t>(gd->hProcess,
                                          gd->clientModuleBaseAddress + hazedumper::signatures::dwEntityList +
                                          ((crosshair - 1) * 0x10));
        auto entityTeam = mem::RPM<uintptr_t>(gd->hProcess, entity + hazedumper::netvars::m_iTeamNum);
        auto myTeam = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
        auto entityHealth = mem::RPM<uintptr_t>(gd->hProcess, entity + hazedumper::netvars::m_iHealth);

        if (entityHealth > 0 and entityTeam != myTeam) { // enemy detect
            shoot(gd);
        } else {
            return 0;
        }
    }
    return 0;
}


/**
 * Recoil Control System, Counter recoil by moving view angle to the opposite direction of the punch angle
 * @param gd
 * @return
 */
int RCS(GameData *gd) {

    // Get clientState because view angle is in clientState
    gd->clientState = mem::RPM<uintptr_t>(gd->hProcess,
                                          gd->engineModuleBaseAddress + hazedumper::signatures::dwClientState);
    Vec3 punchAngle = mem::RPM<Vec3>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_aimPunchAngle);
    Vec3 viewAngle = mem::RPM<Vec3>(gd->hProcess, gd->clientState + hazedumper::signatures::dwClientState_ViewAngles);
    int isShotFired = mem::RPM<int>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iShotsFired);
    if (isShotFired > 1) {
        Vec3 newViewAngle = ((viewAngle + gd->localPlayer.lastPunch - punchAngle * 2.f));
        newViewAngle.Normalize();
        gd->localPlayer.lastPunch = punchAngle * 2.f;

        AimToTargetSmooth(gd, viewAngle, newViewAngle, 100);
    } else {
        gd->localPlayer.lastPunch = {0, 0, 0};
    }

    return 0;
}

Vec3 WorldToScreen(const Vec3 pos, ViewMatrix matrix, int screenX, int screenY) {
    float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
    float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];

    float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

    float inv_w = 1.f / w;
    _x *= inv_w;
    _y *= inv_w;

    float x = screenX * .5f;
    float y = screenY * .5f;

    x += 0.5f * _x * screenX + 0.5f;
    y -= 0.5f * _y * screenY + 0.5f;

    return {x, y, w};
}

void DrawFilledRect(int x, int y, int w, int h, GameData *gd) {
    RECT rect = {x, y, x + w, y + h};
    FillRect(gd->hdc, &rect, gd->boxEspData.enemyBrush);
}

void DrawBorderBox(int x, int y, int w, int h, int thickness, GameData *gd) {
    DrawFilledRect(x, y, w, thickness, gd); //Top horiz line
    DrawFilledRect(x, y, thickness, h, gd); //Left vertical line
    DrawFilledRect((x + w), y, thickness, h, gd); //right vertical line
    DrawFilledRect(x, y + h, w + thickness, thickness, gd); //bottom horiz line
}

void DrawLine(float StartX, float StartY, float EndX, float EndY, GameData *gd) {
    int a, b = 0;
    HPEN hOPen;
    HPEN hNPen = CreatePen(PS_SOLID, 5, EnemyPen);// penstyle, width, color
    hOPen = (HPEN) SelectObject(gd->hdc, hNPen);
    MoveToEx(gd->hdc, StartX, StartY, NULL); //start
    a = LineTo(gd->hdc, EndX, EndY); //end
    DeleteObject(SelectObject(gd->hdc, hOPen));
}

//void BoxEsp(GameData* gd, ViewMatrix vm, uintptr_t targetEnt) {
////    int health = mem::RPM<int>(gd->hProcess, targetEnt + hazedumper::netvars::m_iHealth);
//    Vec3 pos = mem::RPM<Vec3>(gd->hProcess, targetEnt + hazedumper::netvars::m_vecOrigin);
//
//    Vec3 head;
//    head.x = pos.x;
//    head.y = pos.y;
//    head.z = pos.z + 75.f;
//    Vec3 screenPos = WorldToScreen(pos, vm, gd->boxEspData.screenX, gd->boxEspData.screenY);
//    Vec3 screenHead = WorldToScreen(head, vm, gd->boxEspData.screenX, gd->boxEspData.screenY);
//    float height = screenHead.y - screenPos.y;
//    float width = height / 2.4f;
//    if (screenPos.z >= 0.01f) {
//        if (gd->feature.lineEsp) {
//            DrawLine(gd->boxEspData.screenX / 2, gd->boxEspData.screenY, screenPos.x, screenPos.y, gd);//    DrawLine(0, 0, 640, -360, gd);
//        }
//        else {
//            DrawBorderBox(screenPos.x - (width / 2), screenPos.y, width, height, 5, gd);//    DrawLine(gd->boxEspData.screenX / 2, gd->boxEspData.screenY, screenPos.x, screenPos.y, gd);
//        }
//    }
//
//}
//
///**
// * This function loop through all players ent list
// * @param gd GameData Struct
// * @return 0
// */
//int LoopEntList(GameData *gd) {
//    // Glow Object Manager
//    auto glowObject = mem::RPM<uintptr_t>(gd->hProcess,
//                                          gd->clientModuleBaseAddress + hazedumper::signatures::dwGlowObjectManager);
//    auto vm = mem::RPM<ViewMatrix>(gd->hProcess, gd->clientModuleBaseAddress + hazedumper::signatures::dwViewMatrix);
//    auto myTeam = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
//
//    for (int i = 0; i < 64; i++) {
//        auto targetEntity = mem::RPM<uintptr_t>(gd->hProcess,
//                                                gd->clientModuleBaseAddress + hazedumper::signatures::dwEntityList +
//                                                i * 0x10);
//        if (targetEntity) { // if there is an enemy
//            auto targetTeam = mem::RPM<uintptr_t>(gd->hProcess, targetEntity + hazedumper::netvars::m_iTeamNum);
//            int health = mem::RPM<int>(gd->hProcess, targetEntity + hazedumper::netvars::m_iHealth);
//
//            // check whether target is in the enemy team
//            if (targetTeam != myTeam && myTeam <= 9 && health > 0 && health <= 100) {
//                if (gd->feature.glowEsp) {
//                    RadarEsp(gd, targetEntity);
//                    makeEntGlow(gd, glowObject, targetEntity);
//                }
//                if (gd->feature.boxEsp) {
//                    BoxEsp(gd, vm, targetEntity);
//                }
//            }
//        }
//    }
//    return 0;
//}
/**
 * Update current player list
 * @param gd Game data
 */
void UpdatePlayerList(GameData *gd) {
    auto myTeam = mem::RPM<uintptr_t>(gd->hProcess, gd->localPlayer.playerEnt + hazedumper::netvars::m_iTeamNum);
    for (int i = 0; i < 64; i++) {
        auto player = mem::RPM<uintptr_t>(gd->hProcess,
                                           gd->clientModuleBaseAddress + hazedumper::signatures::dwEntityList +
                                           i * 0x10);
        if (player) { // if there is at least one player
            auto playerTeam = mem::RPM<uintptr_t>(gd->hProcess, player + hazedumper::netvars::m_iTeamNum);
            int health = mem::RPM<int>(gd->hProcess, player + hazedumper::netvars::m_iHealth);

            // if player not dead add them to the list
            if (health > 0 && health <= 100) {
                gd->playerList[i].playerEnt = player;
            } else {
                gd->playerList[i].playerEnt = NULL;
            }
        }
    }
}

int MainLogic(GameData *gd, ImGuiConfig &imGuiConfig) {

    printf("\n\nWelcome!\n\n");
    fflush(stdout);
    DWORD dwExit = 0;

    while (GetExitCodeProcess(gd->hProcess, &dwExit) && dwExit == STILL_ACTIVE) {
        // Handle message from window to GUI WINDOW
        HandleWindowMessage(imGuiConfig);
        if (imGuiConfig.done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        SetCheatMenuContent(imGuiConfig);
        // Menu Rendering
        HandleMenuRendering(imGuiConfig);

        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }

        gd->localPlayer.playerEnt = mem::RPM<uintptr_t>(gd->hProcess, gd->clientModuleBaseAddress +
                                                                      hazedumper::signatures::dwLocalPlayer);
        if (imGuiConfig.feature.bHop) {
            BHop(gd);
        }
        if (imGuiConfig.feature.rcs) {
            RCS(gd);
        }
        if (imGuiConfig.feature.antiFlash) {
            AntiFlash(gd);
        }
        if (imGuiConfig.feature.radarEsp || imGuiConfig.feature.glowEsp) {
            UpdatePlayerList(gd); // update player list
        }
        if (imGuiConfig.feature.radarEsp) {
            RadarEsp(gd);
        }
        if (imGuiConfig.feature.glowEsp) {
            GlowEsp(gd);
        }


        if (imGuiConfig.feature.bHop && GetAsyncKeyState(VK_MENU)) { // key to activate TriggerBot
            checkTBot(gd);
        }
        Sleep(5);
    }
    printf("Thank you For Choosing xyz!\n");
    fflush(stdout);
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

    gameData.hdc = GetDC(FindWindowA(NULL, TEXT(pi.clientName)));
    gameData.boxEspData.SetDefaultConfig();

    ImGuiConfig imGuiConfig;
    MainLogic(&gameData, imGuiConfig);
    Sleep(1000);
    return 0;
}

