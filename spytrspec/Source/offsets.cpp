#include "offsets.h"
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include "offsets.hpp"
#include "client_dll.hpp"
#include "offsets.hpp"
#include "client_dll.hpp"


using json = nlohmann::json;

// -------- client.dll offsets --------
uintptr_t offsets::dwEntityList = 0;
uintptr_t offsets::dwLocalPlayerController = 0;

// -------- netvars --------
uintptr_t offsets::m_hPawn = 0;
uintptr_t offsets::m_iszPlayerName = 0;
uintptr_t offsets::m_pObserverServices = 0;
uintptr_t offsets::m_hObserverTarget = 0;

// -------- helper para netvars --------
static uintptr_t getNetvar(
    const json& schema,
    const char* className,
    const char* fieldName
) {
    try {
        const auto& value =
            schema["client.dll"]
            ["classes"]
            [className]
            ["fields"]
            [fieldName];

        uintptr_t offset = value.get<uintptr_t>();

        std::cout << "[NETVAR] "
            << className << "::" << fieldName
            << " = 0x" << std::hex << offset << std::dec << "\n";

        return offset;
    }
    catch (...) {
        std::cout << "[ERRO] Netvar nao encontrada: "
            << className << "::" << fieldName << "\n";
        return 0;
    }
}

bool offsets::load() {
    try {
        // ================= client_dll.json =================
        std::ifstream clientFile("client_dll.json");
        if (!clientFile.is_open()) {
            std::cout << "[ERRO] Nao foi possivel abrir client_dll.json\n";
            return false;
        }

        json jClient;
        clientFile >> jClient;

        if (!jClient.contains("client.dll")) {
            std::cout << "[ERRO] client.dll nao existe no JSON\n";
            return false;
        }

        auto& client = jClient["client.dll"];

        dwEntityList = client["dwEntityList"].get<uintptr_t>();
        dwLocalPlayerController =
            client["dwLocalPlayerController"].get<uintptr_t>();

        std::cout << "[CLIENT] dwEntityList = 0x"
            << std::hex << dwEntityList << "\n";

        std::cout << "[CLIENT] dwLocalPlayerController = 0x"
            << std::hex << dwLocalPlayerController << std::dec << "\n";

        // ================= offsets.json (schema) =================
        std::ifstream schemaFile("offsets.json");
        if (!schemaFile.is_open()) {
            std::cout << "[ERRO] Nao foi possivel abrir offsets.json\n";
            return false;
        }

        json schema;
        schemaFile >> schema;

        if (!schema.contains("client.dll")) {
            std::cout << "[ERRO] client.dll nao existe no schema\n";
            return false;
        }

        // -------- netvars --------
        m_hPawn =
            getNetvar(schema, "CCSPlayerController", "m_hPawn");

        m_iszPlayerName =
            getNetvar(schema, "CCSPlayerController", "m_iszPlayerName");

        m_pObserverServices =
            getNetvar(schema, "C_BasePlayerPawn", "m_pObserverServices");

        m_hObserverTarget =
            getNetvar(schema, "CPlayer_ObserverServices", "m_hObserverTarget");

        std::cout << "\n[OK] TODOS OS OFFSETS CARREGADOS COM SUCESSO\n\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "[EXCEPTION] " << e.what() << "\n";
        return false;
    }
}
