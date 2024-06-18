#pragma once

#include <cstdint>
#include <string>

#include <Windows.h>
#include <psapi.h>
#include <errhandlingapi.h>
#include <DbgHelp.h>
#include "ehdata-structs.hpp"

#define EXCEPTION_SET_THREAD_NAME 0x406D1388
#define EH_EXCEPTION_NUMBER 0xE06D7363
#define RPC_S_SERVER_UNAVAILABLE 0x6BA

namespace analyzer::exceptions {

    /// @brief Convert an exception code to its name.
    const char *getName(DWORD exceptionCode);

    /// @brief Convert a VirtualProtect flag to its name.
    std::string getProtectionString(DWORD protection);

    /// @brief Convert a VirtualQuery state to its name.
    std::string getMemStateString(DWORD state);

    /// @brief Convert a VirtualQuery type to its name.
    std::string getMemTypeString(DWORD type);

    /// @brief Handler for EXCEPTION_ACCESS_VIOLATION
    std::string accessViolationHandler(LPEXCEPTION_POINTERS exceptionInfo);

    /// @brief Handler for EXCEPTION_ILLEGAL_INSTRUCTION
    std::string illegalInstructionHandler(LPEXCEPTION_POINTERS exceptionInfo);

    /// @brief Get extra information about an exception. (if available)
    std::string getExtraInfo(DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo);

    /// @brief Get the parameters of an exception.
    std::string getParameters(DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo);

}