#include "exception-codes.hpp"

#include <fmt/format.h>
#include <sstream>
#include <stdexcept>
#include <Windows.h>

#define GEODE_TERMINATE_EXCEPTION_CODE 0x4000
#define GEODE_UNREACHABLE_EXCEPTION_CODE 0x4001

namespace analyzer::exceptions {

    const char *getName(DWORD exceptionCode) {
#define CASE(code) case code: return #code
        switch (exceptionCode) {
            CASE(EXCEPTION_ACCESS_VIOLATION);
            CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
            CASE(EXCEPTION_BREAKPOINT);
            CASE(EXCEPTION_DATATYPE_MISALIGNMENT);
            CASE(EXCEPTION_FLT_DENORMAL_OPERAND);
            CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO);
            CASE(EXCEPTION_FLT_INEXACT_RESULT);
            CASE(EXCEPTION_FLT_INVALID_OPERATION);
            CASE(EXCEPTION_FLT_OVERFLOW);
            CASE(EXCEPTION_FLT_STACK_CHECK);
            CASE(EXCEPTION_FLT_UNDERFLOW);
            CASE(EXCEPTION_ILLEGAL_INSTRUCTION);
            CASE(EXCEPTION_IN_PAGE_ERROR);
            CASE(EXCEPTION_INT_DIVIDE_BY_ZERO);
            CASE(EXCEPTION_INT_OVERFLOW);
            CASE(EXCEPTION_INVALID_DISPOSITION);
            CASE(EXCEPTION_NONCONTINUABLE_EXCEPTION);
            CASE(EXCEPTION_PRIV_INSTRUCTION);
            CASE(EXCEPTION_SINGLE_STEP);
            CASE(EXCEPTION_STACK_OVERFLOW);
            CASE(EXCEPTION_GUARD_PAGE);
            CASE(EXCEPTION_INVALID_HANDLE);
            CASE(GEODE_TERMINATE_EXCEPTION_CODE);
            CASE(GEODE_UNREACHABLE_EXCEPTION_CODE);
            CASE(EH_EXCEPTION_NUMBER);
            default:
                return "Unknown exception";
        }
#undef CASE
    }

    std::string getProtectionString(DWORD protection) {
        std::string protectionStr;
        if (protection & PAGE_NOACCESS) protectionStr += "PAGE_NOACCESS | ";
        if (protection & PAGE_READONLY) protectionStr += "PAGE_READONLY | ";
        if (protection & PAGE_READWRITE) protectionStr += "PAGE_READWRITE | ";
        if (protection & PAGE_WRITECOPY) protectionStr += "PAGE_WRITECOPY | ";
        if (protection & PAGE_EXECUTE) protectionStr += "PAGE_EXECUTE | ";
        if (protection & PAGE_EXECUTE_READ) protectionStr += "PAGE_EXECUTE_READ | ";
        if (protection & PAGE_EXECUTE_READWRITE) protectionStr += "PAGE_EXECUTE_READWRITE | ";
        if (protection & PAGE_EXECUTE_WRITECOPY) protectionStr += "PAGE_EXECUTE_WRITECOPY | ";
        if (protection & PAGE_GUARD) protectionStr += "PAGE_GUARD | ";
        if (protection & PAGE_NOCACHE) protectionStr += "PAGE_NOCACHE | ";
        if (protection & PAGE_WRITECOMBINE) protectionStr += "PAGE_WRITECOMBINE | ";
        if (!protectionStr.empty()) {
            protectionStr.pop_back();
            protectionStr.pop_back();
            protectionStr.pop_back();
        }
        return protectionStr;
    }

    std::string getMemStateString(DWORD state) {
        std::string stateStr;
        if (state & MEM_COMMIT) stateStr += "MEM_COMMIT | ";
        if (state & MEM_RESERVE) stateStr += "MEM_RESERVE | ";
        if (state & MEM_FREE) stateStr += "MEM_FREE | ";
        if (!stateStr.empty()) {
            stateStr.pop_back();
            stateStr.pop_back();
            stateStr.pop_back();
        }
        return stateStr;
    }

    std::string getMemTypeString(DWORD type) {
        std::string typeStr;
        if (type & MEM_IMAGE) typeStr += "MEM_IMAGE | ";
        if (type & MEM_MAPPED) typeStr += "MEM_MAPPED | ";
        if (type & MEM_PRIVATE) typeStr += "MEM_PRIVATE | ";
        if (!typeStr.empty()) {
            typeStr.pop_back();
            typeStr.pop_back();
            typeStr.pop_back();
        } else {
            typeStr = "Unknown";
        }
        return typeStr;
    }

    std::string accessViolationHandler(LPEXCEPTION_POINTERS exceptionInfo) {
        auto exceptionRecord = exceptionInfo->ExceptionRecord;
        auto accessViolationType = exceptionRecord->ExceptionInformation[0];
        auto accessViolationAddress = exceptionRecord->ExceptionInformation[1];

        std::string accessViolationTypeStr;
        switch (accessViolationType) {
            case 0:
                accessViolationTypeStr = "Read";
                break;
            case 1:
                accessViolationTypeStr = "Write";
                break;
            case 8:
                accessViolationTypeStr = "DEP";
                break;
            default:
                accessViolationTypeStr = "Unknown";
                break;
        }

        std::string accessViolationAddressStr;
        if (accessViolationType == 8) {
            accessViolationAddressStr = fmt::format("0x{:X}", accessViolationAddress);
        } else {
            MEMORY_BASIC_INFORMATION memoryInfo;
            VirtualQuery((LPCVOID) accessViolationAddress, &memoryInfo, sizeof(memoryInfo));
            std::string protection = getProtectionString(memoryInfo.Protect);
            std::string state = getMemStateString(memoryInfo.State);
            std::string type = getMemTypeString(memoryInfo.Type);
            accessViolationAddressStr = fmt::format(
                    "0x{:08X}\n"
                    "- Protect: {} (0x{:X})\n"
                    "- State: {} (0x{:X})\n"
                    "- Type: {} (0x{:X})",
                    accessViolationAddress,
                    protection, memoryInfo.Protect,
                    state, memoryInfo.State,
                    type, memoryInfo.Type
            );
        }

        return fmt::format(
                "- Access Violation Type: {}\n"
                "- Access Violation Address: {}",
                accessViolationTypeStr, accessViolationAddressStr
        );
    }

    std::string illegalInstructionHandler(LPEXCEPTION_POINTERS exceptionInfo) {
        auto exceptionRecord = exceptionInfo->ExceptionRecord;
        auto illegalInstructionAddress = exceptionRecord->ExceptionAddress;
        auto illegalInstructionCode = *(uint16_t *) illegalInstructionAddress;

        return fmt::format(
                "- Illegal Instruction Address: 0x{:X}\n"
                "- Illegal Instruction Code: 0x{:X}",
                (uintptr_t) illegalInstructionAddress, illegalInstructionCode
        );
    }

    template <typename T, typename U>
    static std::add_const_t<std::decay_t<T>> rebaseAndCast(intptr_t base, U value) {
        // U value -> const T* (base+value)
        return reinterpret_cast<std::add_const_t<std::decay_t<T>>>(base + (ptrdiff_t)(value));
    }

    std::string cppExceptionHandler(LPEXCEPTION_POINTERS exceptionInfo) {
        bool isStdException = false;
        auto *exceptionRecord = exceptionInfo->ExceptionRecord;
        auto exceptionObject = exceptionRecord->ExceptionInformation[1];

        intptr_t imageBase = exceptionRecord->NumberParameters >= 4 ? static_cast<intptr_t>(exceptionRecord->ExceptionInformation[3]) : 0;
        auto* throwInfo = reinterpret_cast<_MSVC_ThrowInfo*>(exceptionRecord->ExceptionInformation[2]);
        std::stringstream stream;
        if (!throwInfo || !throwInfo->pCatchableTypeArray) {
            stream << "C++ exception: <no SEH data available about the thrown exception>";
        } else {
            auto* catchableTypeArray = rebaseAndCast<_MSVC_CatchableTypeArray*>(imageBase, throwInfo->pCatchableTypeArray);
            auto ctaSize = catchableTypeArray->nCatchableTypes;
            const char* targetName = nullptr;

            for (int i = 0; i < ctaSize; i++) {
                auto* catchableType = rebaseAndCast<_MSVC_CatchableType*>(imageBase, catchableTypeArray->arrayOfCatchableTypes[i]);
                auto* ctDescriptor = rebaseAndCast<_MSVC_TypeDescriptor*>(imageBase, catchableType->pType);
                const char* classname = ctDescriptor->name;

                if (i == 0) {
                    targetName = classname;
                }

                if (strcmp(classname, ".?AVexception@std@@") == 0) {
                    isStdException = true;
                    break;
                }
            }

            // demangle the name of the thrown object
            std::string demangledName;

            if (!targetName || targetName[0] == '\0' || targetName[1] == '\0') {
                demangledName = "<Unknown type>";
            } else {
                char demangledBuf[256];
                size_t written = UnDecorateSymbolName(targetName + 1, demangledBuf, 256, UNDNAME_NO_ARGUMENTS);
                if (written == 0) {
                    demangledName = "<Unknown type>";
                } else {
                    demangledName = std::string(demangledBuf, demangledBuf + written);
                }
            }

            if (isStdException) {
                auto* excObject = reinterpret_cast<std::exception*>(exceptionObject);
                stream << "C++ Exception: " << demangledName << "(\"" << excObject->what() << "\")" << "";
            } else {
                stream << "C++ Exception: type '" << demangledName << "'";
            }
        }

        return stream.str();
    }

    /// @brief Handler for EXCEPTION_WINE_STUB
    std::string wineStubHandler(LPEXCEPTION_POINTERS exceptionInfo) {
        auto* dll = reinterpret_cast<const char*>(exceptionInfo->ExceptionRecord->ExceptionInformation[0]);
        auto* function = reinterpret_cast<const char*>(exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
        return fmt::format(
            "Attempted to invoke a non-existent function:\n"
            "Mangled name: {}\n",
            "Looking in: {}",
            function, dll
        );
    }

    /// @brief Handler for Geode exceptions
    std::string geodeExceptionHandler(LPEXCEPTION_POINTERS exceptionInfo) {
        auto* reason = reinterpret_cast<const char *>(exceptionInfo->ExceptionRecord->ExceptionInformation[0]);
        auto* mod = reinterpret_cast<geode::Mod*>(exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
        return fmt::format(
            "A mod has deliberately asked the game to crash.\n"
            "Reason: {}\n"
            "Mod: {} ({})",
            reason, mod->getID(), mod->getName()
        );
    }

    std::string getExtraInfo(DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo) {
#define CASE(code, name) case code: return name##Handler(exceptionInfo)
        switch (exceptionCode) {
            CASE(EXCEPTION_ACCESS_VIOLATION, accessViolation);
            CASE(EXCEPTION_ILLEGAL_INSTRUCTION, illegalInstruction);
            CASE(EH_EXCEPTION_NUMBER, cppException);
            CASE(EXCEPTION_WINE_STUB, wineStub);
            CASE(GEODE_TERMINATE_EXCEPTION_CODE, geodeException);
            CASE(GEODE_UNREACHABLE_EXCEPTION_CODE, geodeException);
            default:
                return "";
        }
#undef CASE
    }

    std::string getParameters(DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo) {
        auto exceptionRecord = exceptionInfo->ExceptionRecord;
        auto parameters = exceptionRecord->ExceptionInformation;
        auto parameterCount = exceptionRecord->NumberParameters;

        std::string result;
        for (DWORD i = 0; i < parameterCount; i++) {
            result += fmt::format("0x{:X}, ", parameters[i]);
        }

        if (!result.empty()) {
            result.pop_back();
            result.pop_back();
        }

        return result;
    }

}