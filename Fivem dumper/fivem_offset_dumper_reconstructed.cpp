#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "fivem_sdk_structs.h"

// Extended Offset Target Mapping Structure
struct ExtendedOffsetTarget {
    const char* category;
    const char* name;
    const char* pattern;
    bool is_rip_relative;
    int rip_offset;
    int instruction_size;
    uint32_t static_struct_offset; // Fallback / Static offset inside struct
};

// Complete Signature Matrix for Global & Internal Offsets
static const std::vector<ExtendedOffsetTarget> g_ExtendedTargets = {
    // 1. Top-Level Interface Singletons
    { "GLOBAL SINGLETONS", "world",                  "48 8B 05 ? ? ? ? 33 D2 48 8B 40 08 8A CA 48 85 C0 74 16 48 8B", true, 3, 7, 0x0 },
    { "GLOBAL SINGLETONS", "replay_interface",       "48 8D 0D ? ? ? ? 48 ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? 8A D8 E8 ? ? ? ? 84 DB 75 13 48 8D 0D ? ? ? ? 48 8B D7 E8 ? ? ? ? 84 C0 74 BC 8B 8F", true, 3, 7, 0x0 },
    { "GLOBAL SINGLETONS", "viewport",               "48 8B 15 ? ? ? ? 48 8D 2D ? ? ? ? 48 8B CD", true, 3, 7, 0x0 },
    { "GLOBAL SINGLETONS", "blip_list",              "4C 8D 05 ? ? ? ? 0F B7 C1", true, 3, 7, 0x0 },
    { "GLOBAL SINGLETONS", "camera",                 "4C 8B 35 ? ? ? ? 33 FF 32 DB", true, 3, 7, 0x0 },
    { "GLOBAL SINGLETONS", "bullet",                 "F3 41 0F 10 19 F3 41 0F 10 41 04", false, 0, 0, 0x0 },
    { "GLOBAL SINGLETONS", "aim_cped",               "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74 05 E8 ? ? ? ? 8A CB", true, 3, 7, 0x0 },
    { "GLOBAL SINGLETONS", "set_ped_in_to_vehicle", "48 8B C4 44 89 48 ? 44 89 40 ? 48 89 50 ? 48 89 48 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 83 BA", false, 0, 0, 0x0 },
    { "GLOBAL SINGLETONS", "c_sky_settings",         "48 8D 0D ? ? ? ? E8 ? ? ? ? 83 25 ? ? ? ? 00 48 8D 0D ? ? ? ? F3", true, 3, 7, 0x0 },

    // 2. Internal CPed & CPlayerInfo Member Offsets
    { "CPED LAYOUT",       "CPed::Position",          "", false, 0, 0, offsetof(CPed, Position) },
    { "CPED LAYOUT",       "CPed::Health",            "", false, 0, 0, offsetof(CPed, Health) },
    { "CPED LAYOUT",       "CPed::MaxHealth",         "", false, 0, 0, offsetof(CPed, MaxHealth) },
    { "CPED LAYOUT",       "CPed::Armor",             "", false, 0, 0, offsetof(CPed, Armor) },
    { "CPED LAYOUT",       "CPed::PlayerInfo",        "", false, 0, 0, offsetof(CPed, PlayerInfo) },
    { "CPED LAYOUT",       "CPed::CurrentVehicle",    "", false, 0, 0, offsetof(CPed, CurrentVehicle) },
    { "CPED LAYOUT",       "CPed::WeaponManager",     "", false, 0, 0, offsetof(CPed, WeaponManager) },

    // 3. Internal CVehicle & CHandlingData Member Offsets
    { "CVEHICLE LAYOUT",   "CVehicle::Position",      "", false, 0, 0, offsetof(CVehicle, Position) },
    { "CVEHICLE LAYOUT",   "CVehicle::EngineHealth",  "", false, 0, 0, offsetof(CVehicle, EngineHealth) },
    { "CVEHICLE LAYOUT",   "CVehicle::BodyHealth",    "", false, 0, 0, offsetof(CVehicle, BodyHealth) },
    { "CVEHICLE LAYOUT",   "CVehicle::FuelTankHealth","", false, 0, 0, offsetof(CVehicle, FuelTankHealth) },
    { "CVEHICLE LAYOUT",   "CVehicle::Handling",      "", false, 0, 0, offsetof(CVehicle, Handling) },
    { "CVEHICLE LAYOUT",   "CVehicle::Velocity",      "", false, 0, 0, offsetof(CVehicle, Velocity) },

    // 4. Internal CWeapon & CWeaponInfo Member Offsets
    { "CWEAPON LAYOUT",    "CWeapon::ClipAmmo",       "", false, 0, 0, offsetof(CWeapon, ClipAmmo) },
    { "CWEAPON LAYOUT",    "CWeapon::AmmoCount",      "", false, 0, 0, offsetof(CWeapon, AmmoCount) },
    { "CWEAPON LAYOUT",    "CWeaponInfo::BulletDamage","", false, 0, 0, offsetof(CWeaponInfo, BulletDamage) },
    { "CWEAPON LAYOUT",    "CWeaponInfo::Spread",     "", false, 0, 0, offsetof(CWeaponInfo, Spread) },
    { "CWEAPON LAYOUT",    "CWeaponInfo::Recoil",     "", false, 0, 0, offsetof(CWeaponInfo, Recoil) },

    // 5. Entity Pool Descriptors
    { "ENTITY POOLS",      "CReplayInterface::PedPool",    "", false, 0, 0, offsetof(CReplayInterface, PedPoolHeader) },
    { "ENTITY POOLS",      "CReplayInterface::VehiclePool", "", false, 0, 0, offsetof(CReplayInterface, VehiclePoolHeader) },
    { "ENTITY POOLS",      "CReplayInterface::ObjectPool",  "", false, 0, 0, offsetof(CReplayInterface, ObjectPoolHeader) },
    { "ENTITY POOLS",      "CReplayInterface::PickupPool",  "", false, 0, 0, offsetof(CReplayInterface, PickupPoolHeader) },

    // 6. Viewport & Transformation Matrix
    { "VIEWPORT & MATRIX", "CViewport::ViewMatrix",   "", false, 0, 0, offsetof(CViewport, ViewMatrix) }
};

// Helper: Print centered colored text
void PrintCentered(const std::string& text, WORD color = 11, int width = 80) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    int pad = (width - (int)text.length()) / 2;
    if (pad < 0) pad = 0;

    SetConsoleTextAttribute(hConsole, color);
    std::cout << std::string(pad, ' ') << text << "\n";
    SetConsoleTextAttribute(hConsole, 7);
}

// Convert pattern string to byte vector and mask
void PatternToMask(const std::string& pattern, std::vector<BYTE>& bytes, std::string& mask) {
    bytes.clear();
    mask.clear();
    std::stringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "?") {
            bytes.push_back(0x00);
            mask.push_back('?');
        } else {
            bytes.push_back(static_cast<BYTE>(std::stoul(token, nullptr, 16)));
            mask.push_back('x');
        }
    }
}

// Find process ID and return main executable module handle
DWORD GetFiveMProcessInfo(HANDLE& hProcess, uintptr_t& baseAddr, DWORD& imageSize) {
    baseAddr = 0;
    imageSize = 0;

    // 1. Try finding window handle
    HWND hwnd = FindWindowA("grcWindow", nullptr);
    DWORD pid = 0;
    if (hwnd) {
        GetWindowThreadProcessId(hwnd, &pid);
    }

    // 2. Snapshot fallback
    if (!pid) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32 = { sizeof(PROCESSENTRY32W) };
            if (Process32FirstW(snapshot, &pe32)) {
                do {
                    std::wstring exeName = pe32.szExeFile;
                    if (exeName.find(L"FiveM_GTAProcess.exe") != std::wstring::npos ||
                        exeName.find(L"FiveM.exe") != std::wstring::npos ||
                        exeName.find(L"GTA5.exe") != std::wstring::npos) {
                        pid = pe32.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(snapshot, &pe32));
            }
            CloseHandle(snapshot);
        }
    }

    if (!pid) return 0;

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return 0;

    // Iterate modules to find FiveM_GTAProcess.exe or GTA5.exe base module
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (K32EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        int count = cbNeeded / sizeof(HMODULE);
        for (int i = 0; i < count; i++) {
            char modName[MAX_PATH];
            if (K32GetModuleBaseNameA(hProcess, hMods[i], modName, sizeof(modName))) {
                std::string name = modName;
                if (name.find("FiveM_GTAProcess") != std::string::npos ||
                    name.find("GTA5.exe") != std::string::npos) {
                    MODULEINFO modInfo;
                    if (K32GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                        baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                        imageSize = modInfo.SizeOfImage;
                        return pid;
                    }
                }
            }
        }
        // Fallback to first module if specific name match not triggered
        MODULEINFO modInfo;
        if (K32GetModuleInformation(hProcess, hMods[0], &modInfo, sizeof(modInfo))) {
            baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
            imageSize = modInfo.SizeOfImage;
        }
    }

    return pid;
}

// Chunked memory pattern scanner (reads in 2MB blocks)
uintptr_t ChunkedPatternScan(HANDLE hProcess, uintptr_t baseAddr, DWORD imageSize, const std::vector<BYTE>& pattern, const std::string& mask) {
    size_t patternLen = mask.length();
    const DWORD CHUNK_SIZE = 2 * 1024 * 1024; // 2MB chunking
    std::vector<BYTE> buffer(CHUNK_SIZE + patternLen);

    uintptr_t currentAddr = baseAddr;
    uintptr_t endAddr = baseAddr + imageSize;

    while (currentAddr < endAddr) {
        DWORD bytesToRead = static_cast<DWORD>(min((uintptr_t)CHUNK_SIZE + patternLen, endAddr - currentAddr));
        SIZE_T bytesRead = 0;

        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(currentAddr), buffer.data(), bytesToRead, &bytesRead) && bytesRead >= patternLen) {
            for (size_t i = 0; i <= bytesRead - patternLen; ++i) {
                bool found = true;
                for (size_t j = 0; j < patternLen; ++j) {
                    if (mask[j] != '?' && buffer[i + j] != pattern[j]) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return currentAddr + i;
                }
            }
        }
        currentAddr += CHUNK_SIZE;
    }
    return 0;
}

int main() {
    SetConsoleTitleA("FiveM Master Offset & SDK Dumper - @Bombeule");

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    std::cout << "\n\n";
    PrintCentered("==================================================================", 13); // Light Magenta
    PrintCentered("      ____  _           __  ___   ____  _____ ____  ______        ", 11); // Cyan
    PrintCentered("     / __ \\(_)   _____ /  |/  /  / __ \\/ ___// __ \\/ ____/        ", 11);
    PrintCentered("    / /_/ / / | / / _ \\/ /|_/ /  / / / / /__ / / / / /__          ", 11);
    PrintCentered("   / ____/ /| |/ /  __/ /  / /  / /_/ / ___// /_/ / /___          ", 11);
    PrintCentered("  /_/   /_/ |___/\\___/_/  /_/   \\____/_/    \\____/_____/          ", 11);
    PrintCentered("                                                                  ", 11);
    PrintCentered("          FiveM Master Offset & SDK Dumper - @Bombeule            ", 14); // Yellow
    PrintCentered("==================================================================", 13);
    std::cout << "\n";

    PrintCentered("[*] Searching for FiveM / GTA5 sub-process...", 11);

    HANDLE hProcess = NULL;
    uintptr_t baseAddr = 0;
    DWORD imageSize = 0;
    DWORD pid = 0;

    while ((pid = GetFiveMProcessInfo(hProcess, baseAddr, imageSize)) == 0 || baseAddr == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::stringstream ssPid;
    ssPid << "[+] Sub-Process Attached! (PID: " << pid << ")";
    PrintCentered(ssPid.str(), 10);

    std::stringstream ssBase;
    ssBase << "[*] FiveM_GTAProcess Base: 0x" << std::hex << std::uppercase << baseAddr;
    PrintCentered(ssBase.str(), 14);
    std::cout << "\n";

    std::ofstream outFile("offsets.txt");
    outFile << "==================================================================\n";
    outFile << "        FiveM Master SDK & Class Member Offsets - @Bombeule        \n";
    outFile << "==================================================================\n\n";
    outFile << "Process Base Address: 0x" << std::hex << std::uppercase << baseAddr << "\n\n";

    std::string currentCat = "";

    for (const auto& target : g_ExtendedTargets) {
        if (currentCat != target.category) {
            currentCat = target.category;
            std::cout << "\n";
            PrintCentered("------------------------------------------------------------------", 8);
            PrintCentered("    " + currentCat + "    ", 14);
            PrintCentered("------------------------------------------------------------------", 8);

            outFile << "\n[" << currentCat << "]\n";
        }

        std::stringstream ssRow;
        std::stringstream ssFileRow;

        if (target.pattern[0] != '\0') {
            std::vector<BYTE> bytes;
            std::string mask;
            PatternToMask(target.pattern, bytes, mask);

            uintptr_t matchAddr = ChunkedPatternScan(hProcess, baseAddr, imageSize, bytes, mask);
            if (matchAddr != 0) {
                uintptr_t finalOffset = 0;
                if (target.is_rip_relative) {
                    int32_t disp = 0;
                    ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(matchAddr + target.rip_offset), &disp, sizeof(disp), nullptr);
                    uintptr_t resolvedVA = matchAddr + target.instruction_size + disp;
                    finalOffset = resolvedVA - baseAddr;
                } else {
                    finalOffset = matchAddr - baseAddr;
                }

                ssRow << std::left << std::setw(32) << target.name << ": 0x" << std::hex << std::uppercase << finalOffset;
                ssFileRow << std::left << std::setw(32) << target.name << ": 0x" << std::hex << std::uppercase << finalOffset;
                PrintCentered(ssRow.str(), 10);
            } else {
                ssRow << std::left << std::setw(32) << target.name << ": NOT FOUND";
                ssFileRow << std::left << std::setw(32) << target.name << ": NOT FOUND";
                PrintCentered(ssRow.str(), 12);
            }
        } else {
            ssRow << std::left << std::setw(32) << target.name << ": 0x" << std::hex << std::uppercase << target.static_struct_offset;
            ssFileRow << std::left << std::setw(32) << target.name << ": 0x" << std::hex << std::uppercase << target.static_struct_offset;
            PrintCentered(ssRow.str(), 11);
        }

        outFile << ssFileRow.str() << "\n";
    }

    outFile << "\n==================================================================\n";
    outFile.close();

    std::cout << "\n";
    PrintCentered("------------------------------------------------------------------", 8);
    PrintCentered("[+] All Master Global & Internal Offsets saved to 'offsets.txt'!", 10);
    PrintCentered("------------------------------------------------------------------", 8);
    std::cout << "\n";
    PrintCentered("Press ENTER to exit...", 14);

    std::cin.get();
    CloseHandle(hProcess);
    return 0;
}
