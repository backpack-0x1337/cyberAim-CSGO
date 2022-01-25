
#include "proc.h"
#include <cstdio>
#include <tchar.h>

#include <vector>
#include <iostream>
#include <Windows.h> // <> look for it in project dependency
#include <vector> // inside the project folder


/**
 * Get Process ID with the given Process name
 * @param procName Target process Name
 * @return Target Process ID
 */
DWORD GetProcId(LPCSTR procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!lstrcmpi(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;

}

/**
 * Get Module Base Address of the target process
 * @param procId Target Process ID
 * @param modName Target Process name
 * @return Mod Base Address
 */
uintptr_t GetModuleBaseAddress(DWORD procId, LPCSTR modName) {
    uintptr_t modBaseAddress = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);

        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!lstrcmpi(modEntry.szModule, modName)) {
                    modBaseAddress = (uintptr_t)(modEntry.modBaseAddr);
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddress;

}


/**
 * Find dynamic address via base address and offsets
 * @param hProc program handle
 * @param dbaPtr dynamic base address pointer
 * @param offsets A list of offset
 * @return
 */
uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t dbaPtr,
                      std::vector<unsigned int> offsets) {
    uintptr_t addr = dbaPtr;
    for (unsigned int i = 0; i < offsets.size(); ++i) {
        ReadProcessMemory(hProc, (BYTE *) addr,&addr, sizeof(addr), 0);
        addr += offsets[i];
    }
    return addr;
}


//int main() {
//
//    // Get proId of the target process
//    DWORD procId = GetProcId("ac_client.exe");
//    std::cout << "Process ID = " << procId << std::endl;
//
//    // Get Module address
//    uintptr_t moduleBaseAddress = GetModuleBaseAddress(procId,"ac_client.exe");
//    std::cout << "moduleBaseAddress = " << moduleBaseAddress << std::endl;
//
//    // Get the Handle to process
//    HANDLE hProcess = 0;
//    hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);
//
//    // Resolve base address of the pointer chain
//    uintptr_t dynamicPtrBaseAddress = moduleBaseAddress + 0x192414;
//
//    std::cout << "Dynamic Base Address = " << "0x" << std::hex << dynamicPtrBaseAddress << std::endl;
//
//    std::vector<unsigned int> ammoOffsets = {0x364, 0x14, 0x0};
//    uintptr_t ammoAddress = FindDMAAddy(hProcess, dynamicPtrBaseAddress, ammoOffsets);
//
//    std::cout << "Ammo Address = " << "0x" << std::hex << ammoAddress << std::endl;
//
//    // Read the Ammo Value
//    int ammoValue = 0;
//
//    ReadProcessMemory(hProcess, (BYTE*)ammoAddress,
//                      &ammoValue, sizeof(ammoValue), nullptr);
//
//    std::cout << "Ammo value = " << std::dec << ammoValue << std::endl;
//
//    // Write to it
//    // The
//    int newAmmo = 1337;
//    WriteProcessMemory(hProcess, (BYTE*)ammoAddress,
//                       &newAmmo, sizeof(newAmmo), nullptr);
//    std::cout << "Ammo value = " << std::dec << newAmmo << std::endl;
//
//
//    getchar();
//
//}