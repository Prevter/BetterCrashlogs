#include "exception-codes.hpp"

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

    std::string getExtraInfo(DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo) {
#define CASE(code, name) case code: return name##Handler(exceptionInfo)
        switch (exceptionCode) {
            CASE(EXCEPTION_ACCESS_VIOLATION, accessViolation);
            CASE(EXCEPTION_ILLEGAL_INSTRUCTION, illegalInstruction);
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