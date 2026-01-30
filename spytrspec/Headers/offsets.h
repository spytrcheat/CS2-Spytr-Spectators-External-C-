#pragma once
#include <cstdint>

namespace offsets {
    // client.dll
    extern uintptr_t dwEntityList;
    extern uintptr_t dwLocalPlayerController;

    // netvars
    extern uintptr_t m_hPawn;
    extern uintptr_t m_iszPlayerName;
    extern uintptr_t m_pObserverServices;
    extern uintptr_t m_hObserverTarget;

    bool load();
}
