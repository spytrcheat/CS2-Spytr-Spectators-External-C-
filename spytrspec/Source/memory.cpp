#pragma once
#include <windows.h>
#include <cstdint>

namespace mem {

    inline HANDLE open_process(DWORD pid) {
        return OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    }

    template<typename T>
    inline T read(HANDLE proc, uintptr_t addr) {
        T buffer{};
        ReadProcessMemory(proc, (LPCVOID)addr, &buffer, sizeof(T), nullptr);
        return buffer;
    }

    inline void read_buffer(HANDLE proc, uintptr_t addr, void* buffer, size_t size) {
        ReadProcessMemory(proc, (LPCVOID)addr, buffer, size, nullptr);
    }

}
