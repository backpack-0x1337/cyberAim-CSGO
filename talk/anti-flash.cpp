#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "../proc/proc.h"
#include "../mem/mem.h"


int main() {
    LPCTSTR gameName = "";
    DWORD procID = GetProcId(gameName);
    if (!procID) {
        return 1;
    }
    printf("ProcID: %lu\n", procID);
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, NULL, procID);

    auto flashDuration = mem::RPM<uintptr_t>(handle, 0x100);
    if (flashDuration > 0) {
        printf("stunned please help");
        int temp = 0;
        mem::WPM(handle, 0x100, temp);
    }
}