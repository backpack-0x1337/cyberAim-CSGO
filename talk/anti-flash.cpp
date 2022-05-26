#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "../proc/proc.h"
#include "../mem/mem.h"

//"651EB540 + 0x10470"

int main() {
    LPCTSTR gameName = "csgo.exe";
    DWORD procId = 0;
    procId = GetProcId(gameName);
    if (!procId) {
        return 1;
    }
    std::cout << "Process ID: " << procId << std::endl;


    // Open handle
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
    uintptr_t flashAddr = 0x651eb540 + 0x10470;
    while(TRUE) {
        auto flashDuration = mem::RPM<uintptr_t>(handle, flashAddr);
        if (flashDuration > 0) {
            printf("Stunned Please help");
            int temp = 0;
            mem::WPM(handle, flashAddr, temp);
            fflush(stdout);
            break;
        }
    }
    printf("end");
    getc(stdin);
    return 0;
}