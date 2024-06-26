#pragma once

#include <cstdint>
#include <Windows.h>
#include <psapi.h>
#include <fmt/format.h>
#include <filesystem>

namespace utils::mem {

    /// @brief Check if the address is accessible.
    inline bool isAccessible(uintptr_t address) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery((void*) address, &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        return mbi.State == MEM_COMMIT;
    }

    /// @brief Check if the address is a valid string pointer.
    inline bool isStringPtr(uintptr_t address) {
        if (!isAccessible(address)) return false;

        // Check if the string is null-terminated
        __try {
            if ((const char*)address == nullptr || strlen((const char*)address) >= 1024) {
                return false;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        // Check if the string is printable
        char *ptr = (char *) address;
        while (*ptr) {
            if (*ptr < 32 && *ptr != '\n' && *ptr != '\r') {
                return false;
            }
            ptr++;
        }
        return true;
    }

    /// @brief Check if the address is a valid function pointer.
    inline bool isFunctionPtr(uintptr_t address) {
        if (!isAccessible(address)) return false;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery((void*) address, &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        return mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
    }

    /// @brief Get the module handle from an address.
    inline HMODULE getModuleHandle(uintptr_t address) {
        HMODULE hModule = nullptr;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)address, &hModule);
        return hModule;
    }

    /// @brief Get the module name from a handle.
    inline std::string getModuleName(HMODULE module, bool stripPath = true) {
        static char buffer[MAX_PATH];
        if (!GetModuleFileName(module, buffer, MAX_PATH))
            return "<Unknown>";
        if (buffer[0] == 0)
            return fmt::format("<Unknown: 0x{:X}>", (uintptr_t) module);
        if (stripPath)
            return std::filesystem::path(buffer).filename().string();
        return buffer;
    }

    /// @brief Get the address of a function by backtracking until we get a 0xCC55 (int 3, push ebp) sequence.
    /// @param address The address to start from.
    /// @param maxOffset The maximum offset to search for (for safety).
    /// @return The address of the "push ebp" instruction if found, otherwise 0.
    inline uintptr_t findMethodStart(uintptr_t address, uintptr_t maxOffset = 0x1000) {
        uintptr_t offset = 0;
        while (offset < maxOffset) {
            uint16_t instruction = *(uint16_t*) (address - offset);
#ifndef _WIN64
            // int 3, (push ebp | jmp) sequence
            // jmp is used because some functions may be hooked
            if (instruction == 0x55CC || instruction == 0xE9CC) {
                return address - offset + 1;
            }
#else
            // int 3, (push rbx | jmp) sequence
            // or int 3, (mov [rsp+...], rbx | jmp) sequence
            if (instruction == 0x40CC || instruction == 0x48CC || instruction == 0xE9CC) {
                return address - offset + 1;
            }
#endif
            offset++;
        }
        return 0;
    }

    /// @brief Write memory to an address.
    /// @param address The address to write to.
    /// @param buffer The buffer to write.
    /// @param size The size of the buffer.
    /// @return Whether the write was successful.
    inline bool writeMemory(uintptr_t address, const void* buffer, size_t size) {
        DWORD oldProtect;
        if (!VirtualProtect((void*) address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }
        memcpy((void*) address, buffer, size);
        VirtualProtect((void*) address, size, oldProtect, &oldProtect);
        return true;
    }

}