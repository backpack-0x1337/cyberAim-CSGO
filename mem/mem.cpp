

#include <Windows.h> // <> look for it in project dependency
#include "mem.h"

#define JMP 0xE9;


/**
 * Patch the instruction at given address
 * @param dst
 * @param src
 * @param size
 * @param hProcess
 */
void mem::PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess) {
    DWORD oldProtect;
    VirtualProtectEx(hProcess, dst ,size, PAGE_EXECUTE_READWRITE, &oldProtect);
    WriteProcessMemory(hProcess, dst, src, size, nullptr);
    VirtualProtectEx(hProcess, dst ,size, oldProtect, &oldProtect);
}

/**
 * Nop the instruction in the given location for given size
 * @param dst
 * @param size
 * @param hProcess
 */
void mem::NopEx(BYTE *dst, unsigned int size, HANDLE hProcess) {

    BYTE* nopArray = new BYTE[size];
    memset(nopArray, 0x90, size);

    PatchEx(dst, nopArray, size, hProcess);
    delete[] nopArray;
}

/**
 * Read specific type at given address
 * @tparam T
 * @param hProcess Handle to the process
 * @param address Address to red from
 * @return return the value at given location
 */
template<typename T> T RPM(HANDLE hProcess, uintptr_t address) {
    T buffer;
    ReadProcessMemory(hProcess, (BYTE*)address, &buffer, sizeof(T), NULL);
    return buffer;
}

/**
 * Write specific type value at given address
 * @tparam T
 * @param hProcess Handle to the process
 * @param address Address to red from
 * @return return the value that was written
 */
template<typename T> T WPM(HANDLE hProcess, uintptr_t address, T buffer) {
    WriteProcessMemory(hProcess, (BYTE*)address, &buffer, sizeof(T), NULL);
    return buffer;
}

void mem::Patch(BYTE* dst, BYTE* src, unsigned int size){
    DWORD oldProtect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldProtect, &oldProtect);
}

void mem::Nop(BYTE* dst, unsigned int size) {
    DWORD oldProtect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset(dst, 0x90, size);
    VirtualProtect(dst, size, oldProtect, &oldProtect);
}
uintptr_t mem::FindDMAAddy(uintptr_t dbaPtr, std::vector<unsigned int>offsets) {
    uintptr_t addr = dbaPtr;
    for (unsigned int i = 0; i < offsets.size(); ++i) {
    addr = *(uintptr_t*)addr;
    addr += offsets[i];
    }
    return addr;
}

bool mem::Detour32(BYTE* src, BYTE* dst, const uintptr_t len) {

    if (len < 5) return false;

    DWORD curProtection;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

    uintptr_t relativeAddress = dst - src - 5; // todo  how come dst address is in BYTE??

    *src = 0xE9; // Turn the first instruction into jmp op code

    // Since jump instruction is 1 byte in size and therefore we increase the ptr by one and add the destination address
    *(uintptr_t*)(src + 1) = relativeAddress;

    VirtualProtect(src, len, curProtection, &curProtection);

    return true;
}
BYTE* mem::TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len) {
    if (len < 5) return 0;

    // Create Gateway
    // todo why its not len plus 1?
    BYTE* gateway = (BYTE*)(VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

    // write the stolen byte to the gateway
    // mem copy secure
    memcpy_s(gateway, len, src, len);

    // get the gateway to destination address
    // because we are jumping back up to the original
    uintptr_t gatewayRelativeAddress = src - gateway - 5;

    // add the jmp opcode to the end of gateway
    // todo how come we can derefence this one without converting it to uintptr_t??
    // guess: because JMP is 0xe9 which is a byte hence we dont need to convert it
    // however getwayRelativeAddress is unitptr_r hence we need to convert it to storage of uintptr*
    *(gateway + len) = JMP;

    // add the jmp address for jmp instruction
    *(uintptr_t*)(gateway + len + 1) = gatewayRelativeAddress;

    // perform the detour
    // todo what the fuck does it mean??
    Detour32(src, dst, len);

    return gateway;
}