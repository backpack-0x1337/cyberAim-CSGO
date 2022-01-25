#pragma once

#include <vector>
#include <Windows.h>
#include <TlHelp32.h>


DWORD GetProcId(LPCSTR procName);

uintptr_t GetModuleBaseAddress(DWORD procId, LPCSTR procName);

uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t dbaPtr, std::vector<unsigned int>offsets);
