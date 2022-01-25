//#include <Windows.h>
//#include "../proc/proc.h"
//#include "../csgo.hpp"
//#include "bhop.h"
//
//
//int bhop::activate(GameData gd) {
//    gd->localPlayer.playerEnt = FindDMAAddy(gd->hProcess, gd->moduleBaseAddress + hz::sig::dwLocalPlayer,{0});
//    ReadProcessMemory(gd->hProcess,
//                      (BYTE*) (gd->localPlayer.playerEnt + hz::netvars::m_fFlags),
//                      &gd->localPlayer.flags,
//                      sizeof(gd->localPlayer.flags),
//                      NULL);
//
//    if (GetAsyncKeyState(VK_SPACE) && gd->localPlayer.flags & (1 << 0)) {
//        int x = 6;
//        WriteProcessMemory(gd->hProcess,
//                           (BYTE*)gd->moduleBaseAddress + hz::sig::dwForceJump,
//                           &x,
//                           sizeof(x),
//                           NULL);
//    }
//}