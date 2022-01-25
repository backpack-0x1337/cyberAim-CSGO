//dllMain.cpp
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <tchar.h>

//#include "proc/proc.h"
#include "../mem/mem.h"


// todo what does this shit mean?
typedef BOOL(__stdcall* twglSwapBuffers)(HDC hDc);

twglSwapBuffers owglSwapBuffers; // original function

uintptr_t moduleBaseAddress = (uintptr_t)(GetModuleHandle("ac_client.exe"));

bool bHealth = false, bAmmo = false, bRecoil = false;
uintptr_t ammoAddress = moduleBaseAddress + 0xc526f;
uintptr_t localPlayerPtr = (uintptr_t) moduleBaseAddress + 0x0017B0B8;
uintptr_t healthAddress = mem::FindDMAAddy(localPlayerPtr, {0x35c, 0x18, 0x200, 0x8, 0xec});


BOOL __stdcall hkwglSwapBuffers(HDC hDc) { // Hook function

    if (GetAsyncKeyState(VK_NUMPAD1) & 1)
    {
        bHealth = !bHealth;
        if (bHealth)
        {
            std::cout << "Health Hack activated" << std::endl;
        }
        else
        {
            int* health = (int*)healthAddress;
            *health = 100;
            std::cout << "Health Hack Deactivated" << std::endl;

        }
    }

    if (GetAsyncKeyState(VK_NUMPAD2) & 1)
    {
        bAmmo = !bAmmo;
        if (bAmmo)
        {
            mem::Patch((BYTE*)ammoAddress, (BYTE*)"\xFF\x00", 2);
            std::cout << "Ammo Hack activated" << std::endl;
        }
        else
        {
            mem::Patch((BYTE*)ammoAddress, (BYTE*)"\xFF\x08", 2);
            std::cout << "Ammo Hack Deactivated" << std::endl;
        }
    }

    // continous change value if cheat is activated
    if (*(int*)localPlayerPtr) {
        if (bHealth) {
            int* health = (int*)healthAddress;
            *health = 1337;
        }
    }
    return owglSwapBuffers(hDc);
}


DWORD WINAPI HackThread(HMODULE hModule)
{
    //Create Console
    AllocConsole();
    FILE *f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "FIRST INTERNAL HACK By K\n";


    //
      owglSwapBuffers = (twglSwapBuffers)GetProcAddress(GetModuleHandle("opengl32.dll"),"wglSwapBuffers");
      owglSwapBuffers = (twglSwapBuffers)mem::TrampHook32((BYTE*)owglSwapBuffers, (BYTE*) hkwglSwapBuffers, 5);
      //


//    while (true) {
//        if (GetAsyncKeyState(VK_END) & 1)
//        {
//            break;
//        }
//        if (GetAsyncKeyState(VK_INSERT) & 1)
//        {
//            break;
//        }
//
//        if (GetAsyncKeyState(VK_NUMPAD1) & 1)
//        {
//            bHealth = !bHealth;
//            if (bHealth)
//            {
//                std::cout << "Health Hack activated" << std::endl;
//            }
//            else
//            {
//                int* health = (int*)healthAddress;
//                *health = 100;
//                std::cout << "Health Hack Deactivated" << std::endl;
//
//            }
//        }
//
//        if (GetAsyncKeyState(VK_NUMPAD2) & 1)
//        {
//            bAmmo = !bAmmo;
//            if (bAmmo)
//            {
//                mem::Patch((BYTE*)ammoAddress, (BYTE*)"\xFF\x00", 2);
//                std::cout << "Ammo Hack activated" << std::endl;
//            }
//            else
//            {
//                mem::Patch((BYTE*)ammoAddress, (BYTE*)"\xFF\x08", 2);
//                std::cout << "Ammo Hack Deactivated" << std::endl;
//            }
//        }
//
//        // continous change value if cheat is activated
//        if (*(int*)localPlayerPtr) {
//            if (bHealth) {
//                int* health = (int*)healthAddress;
//                *health = 1337;
//            }
//        }
//        Sleep(5);
//    }
//    fclose(f);
//    FreeConsole();
//    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
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


