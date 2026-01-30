#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <cstdint>

#include "client_dll.hpp"
#include "offsets.hpp"
#include "memory.h"

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets::client_dll;

constexpr int MAX_PLAYERS = 64;
constexpr uintptr_t ENTITY_STRIDE = 0x70;

// ------------------------------------------------------------
// Resolve CHandle<CBaseEntity> -> ponteiro real
uintptr_t GetEntityFromHandle(
    HANDLE proc,
    uintptr_t entityList,
    uint32_t handle
) {
    if (!handle || handle == 0xFFFFFFFF)
        return 0;

    uintptr_t entry = read_memory<uintptr_t>(
        proc,
        entityList + 0x8 * ((handle & 0x7FFF) >> 9) + 0x10
    );
    if (!entry) return 0;

    return read_memory<uintptr_t>(
        proc,
        entry + ENTITY_STRIDE * (handle & 0x1FF)
    );
}

// ------------------------------------------------------------
// Utils
DWORD get_process_id(const wchar_t* name) {
    PROCESSENTRY32W pe{ sizeof(pe) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (!_wcsicmp(pe.szExeFile, name)) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

uintptr_t get_module_base(DWORD pid, const wchar_t* mod) {
    MODULEENTRY32W me{ sizeof(me) };
    HANDLE snap = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

    if (snap == INVALID_HANDLE_VALUE) return 0;

    uintptr_t base = 0;
    if (Module32FirstW(snap, &me)) {
        do {
            if (!_wcsicmp(me.szModule, mod)) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

// ------------------------------------------------------------

int main() {
    std::cout << "[Spectator List - USER MODE]\n\n";

    DWORD pid = get_process_id(L"cs2.exe");
    if (!pid) {
        std::cout << "[ERRO] cs2.exe nao encontrado\n";
        std::cin.get();
        return 1;
    }

    HANDLE proc = open_process(pid);
    if (!proc) {
        std::cout << "[ERRO] Falha ao abrir processo\n";
        std::cin.get();
        return 1;
    }

    uintptr_t client = get_module_base(pid, L"client.dll");
    if (!client) {
        std::cout << "[ERRO] client.dll nao encontrado\n";
        std::cin.get();
        return 1;
    }

    const uintptr_t entityListAddr = client + dwEntityList;
    const uintptr_t localCtrlAddr = client + dwLocalPlayerController;

    Sleep(1500);

    while (true) {
        system("cls");
        std::cout << "[Spectator List]\n\n";

        uintptr_t entityList = read_memory<uintptr_t>(proc, entityListAddr);
        uintptr_t localCtrl = read_memory<uintptr_t>(proc, localCtrlAddr);

        if (!entityList || !localCtrl) {
            std::cout << "Aguardando entrar na partida...\n";
            Sleep(1000);
            continue;
        }

        // ----------------------------------------------------
        // LOCAL PLAYER (VOCÊ)
        uint32_t localPawnHandle =
            read_memory<uint32_t>(proc,
                localCtrl + CCSPlayerController::m_hPlayerPawn);

        uintptr_t localPawn =
            GetEntityFromHandle(proc, entityList, localPawnHandle);

        if (!localPawn) {
            Sleep(1000);
            continue;
        }

        uint8_t localLifeState =
            read_memory<uint8_t>(proc,
                localPawn + C_BaseEntity::m_lifeState);

        if (localLifeState != 0) {
            std::cout << "Voce esta morto (sem espectadores)\n";
            Sleep(1000);
            continue;
        }

        bool found = false;

        // ----------------------------------------------------
        // LOOP EM TODOS OS PLAYERS
        for (int i = 1; i < MAX_PLAYERS; i++) {

            uintptr_t entry = read_memory<uintptr_t>(
                proc,
                entityList + 0x8 * (i >> 9) + 0x10
            );
            if (!entry) continue;

            uintptr_t controller =
                read_memory<uintptr_t>(proc,
                    entry + ENTITY_STRIDE * (i & 0x1FF));

            if (!controller || controller == localCtrl)
                continue;

            // Pawn do jogador
            uint32_t pawnHandle =
                read_memory<uint32_t>(proc,
                    controller + CCSPlayerController::m_hPlayerPawn);

            uintptr_t pawn =
                GetEntityFromHandle(proc, entityList, pawnHandle);

            if (!pawn) continue;

            // Só jogador morto pode spectar
            uint8_t lifeState =
                read_memory<uint8_t>(proc,
                    pawn + C_BaseEntity::m_lifeState);

            if (lifeState == 0)
                continue;

            // ObserverServices (NO PAWN)
            uintptr_t observerServices =
                read_memory<uintptr_t>(proc,
                    pawn + C_BasePlayerPawn::m_pObserverServices);

            if (!observerServices)
                continue;

            uint8_t observerMode =
                read_memory<uint8_t>(proc,
                    observerServices + CPlayer_ObserverServices::m_iObserverMode);

            if (observerMode != 2 && observerMode != 4 && observerMode != 5)
                continue;

            uint32_t targetHandle =
                read_memory<uint32_t>(proc,
                    observerServices + CPlayer_ObserverServices::m_hObserverTarget);

            uintptr_t targetPawn =
                GetEntityFromHandle(proc, entityList, targetHandle);

            if (targetPawn != localPawn)
                continue;

            // Nome
            uintptr_t namePtr =
                read_memory<uintptr_t>(proc,
                    controller + CCSPlayerController::m_sSanitizedPlayerName);

            char name[32]{};
            read_buffer(proc, namePtr, name, sizeof(name));

            std::cout << " • " << name << " (spectating you)\n";
            found = true;
        }

        if (!found)
            std::cout << "\nNenhum espectador\n";

        Sleep(1000);
    }

    return 0;
}
