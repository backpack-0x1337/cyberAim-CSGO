#include <iostream>
#include <Windows.h>
#include "proc/proc.h"
#include "mem/mem.h"

enum Error{
    ERROR_OK,
    ERROR_PROCESS_NOT_FOUND,
    ERROR_INVALID_PLAYER_PTR,
};

Error LogError (Error err) {
    if (err == ERROR_PROCESS_NOT_FOUND) {
        std::cerr << "Process Not Found\n";
        getchar();
        return err;
    }
    return ERROR_OK;
}

struct Location {
    double x,y,z, yaw, pitch;
    bool stored;
};

struct Player {
    uintptr_t addr, healthPtr, currentWeaponAmmoP;
    Location storeLocation;
};

/**
 * store Current Player Location into player struct
 * @param hProcess
 * @param moduleBaseAddress
 * @param localPlayer
 * @return
 */
Error StorePlayerLocation(HANDLE hProcess, Player* localPlayer) {
    double* array[5] = {&localPlayer->storeLocation.x,
                       &localPlayer->storeLocation.y,
                       &localPlayer->storeLocation.z,
                       &localPlayer->storeLocation.yaw,
                       &localPlayer->storeLocation.pitch};
    for (int i = 0; i < 5; i ++) {
        uintptr_t readTarget = localPlayer->addr + 0x34 + 0x4 * i;
        ReadProcessMemory(hProcess, (BYTE*)readTarget,&*array[i], sizeof(readTarget), 0);
    }

    std::cout << "Location x y z stored" << std::endl;
    return ERROR_OK;

}

/**
 * Rewrite player location and view angle
 * @param hProcess
 * @param localPlayer
 * @return
 */
Error TeleportPlayer(HANDLE hProcess, Player* localPlayer) {
    double* array[5] = {&localPlayer->storeLocation.x,
                        &localPlayer->storeLocation.y,
                        &localPlayer->storeLocation.z,
                        &localPlayer->storeLocation.yaw,
                        &localPlayer->storeLocation.pitch};
    for (int i = 0; i < 5; i ++) {
        uintptr_t writeTarget = localPlayer->addr + 0x34 + 0x4 * i;
        mem::PatchEx((BYTE*)writeTarget,
                     (BYTE*)&*array[i], sizeof(writeTarget), hProcess);
    }
    return ERROR_OK;
}

int Hack(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    DWORD dwExit = 0;
    unsigned int hackValue = 1000;
    bool bAmmo = false, bHealth = false;

    Player localPlayer{};
    localPlayer.addr = FindDMAAddy(hProcess, moduleBaseAddress + 0x10f4f4, {0});
    localPlayer.healthPtr = localPlayer.addr + 0xf8;
    localPlayer.currentWeaponAmmoP = localPlayer.addr + 0x150;
    localPlayer.storeLocation.stored = false;

    while (GetExitCodeProcess(hProcess, &dwExit) && dwExit == STILL_ACTIVE) {

        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            bHealth = !bHealth;
            if (bHealth) {
                std::cout << "health Hack activated"<< std::endl;
            } else {
                std::cout << "health Hack deactivated"<< std::endl;
            }
        }
        if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
            bAmmo = !bAmmo;
            if (bAmmo) {
                std::cout << "ammo Hack activated"<< std::endl;
            } else {
                std::cout << "ammo Hack deactivated"<< std::endl;
            }

//            if (bAmmo) {
//                // ff 00 = inc[esi]
//                mem::PatchEx((BYTE*)(moduleBaseAddress + 0xc526f), (BYTE*)"\xFF\x00", 2, hProcess);
//                std::cout << "ammo Hack activated"<< std::endl;
//            } else {
//                // ff 08= dec [esi]
//                mem::PatchEx((BYTE*)(moduleBaseAddress + 0xc526f), (BYTE*)"\xFF\x08", 2, hProcess);
//                std::cout << "ammo Hack deactivate"<< std::endl;
//            }
        }
        if (GetAsyncKeyState(VK_END) & 1) {
            return LogError(ERROR_OK);
        }
        if (GetAsyncKeyState(VK_HOME) & 1) {
            StorePlayerLocation(hProcess, &localPlayer);
            localPlayer.storeLocation.stored = true;
        }
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            if (!localPlayer.storeLocation.stored) {
                std::cout << "location not stored" << std::endl;
                continue;
            }
            TeleportPlayer(hProcess, &localPlayer);
            std::cout << "Teleported" << std::endl;
        }

        // continuous write or freeze
        if (bHealth) {
            mem::PatchEx((BYTE*)localPlayer.healthPtr,
                         (BYTE*)&hackValue, sizeof(hackValue), hProcess);
        }
        if (bAmmo) {
            mem::PatchEx((BYTE*)localPlayer.currentWeaponAmmoP,
                         (BYTE*)&hackValue, sizeof(hackValue), hProcess);
        }
        Sleep(30);

    }
    return LogError(ERROR_OK);
}

int main() {

    HANDLE hProcess = 0;
    uintptr_t moduleBaseAddress = 0;
    Error err = ERROR_OK;

    DWORD procId = 0;
    LPCTSTR clientName = "ac_client.exe";

    procId = GetProcId(clientName);
    if (!procId) {
        return LogError(ERROR_PROCESS_NOT_FOUND);
    }
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);

    moduleBaseAddress = GetModuleBaseAddress(procId, clientName);
    std::cout << "Module Base address: 0x" << std::hex << moduleBaseAddress << std::endl;
    Hack(hProcess, moduleBaseAddress);

    return 0;
}

