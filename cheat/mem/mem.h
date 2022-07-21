
#pragma once
#include <Windows.h> // <> look for it in project dependency
#include <vector>

namespace mem {
    void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess);
    void NopEx(BYTE* dst, unsigned int size, HANDLE hProcess);
    template <class val>
    val ReadMemoryEx(HANDLE hProcess, uintptr_t location);
    template <class val>
    val WriteToMemory(HANDLE hProcess, uintptr_t location, val x);

    template<typename T> T RPM(HANDLE hProcess, uintptr_t address) {
        T buffer;
        ReadProcessMemory(hProcess, (BYTE*)address, &buffer, sizeof(T), NULL);
        return buffer;
    }

    template<typename T> T WPM(HANDLE hProcess, uintptr_t address, T buffer) {
        WriteProcessMemory(hProcess, (BYTE*)address, &buffer, sizeof(T), NULL);
        return buffer;
    }


    void Patch(BYTE* dst, BYTE* src, unsigned int size);
    void Nop(BYTE* dst, unsigned int size);
    uintptr_t FindDMAAddy(uintptr_t dbaPtr, std::vector<unsigned int>offsets);

    bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len);
    BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len);


}
