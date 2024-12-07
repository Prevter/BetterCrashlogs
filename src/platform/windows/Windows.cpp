#if defined(_WIN32) || defined(_WIN64)
#include "Windows.hpp"

#include <functional>

#include "../../crashhandler/crashhandler.hpp"

#define GEODE_TERMINATE_EXCEPTION_CODE 0x4000
#define GEODE_UNREACHABLE_EXCEPTION_CODE 0x4001

namespace breakdown::platform {

    std::function<void(CrashHandler const&)> s_crashHandler;

    std::string_view getExceptionCodeName(DWORD code) {
#define CASE(code) case code: return #code
        switch (code) {
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
            // CASE(EH_EXCEPTION_NUMBER);
            default: return "Unknown Exception";
        }
#undef CASE
    }

    void handleCrash(PEXCEPTION_POINTERS info) {
        auto addr = info->ExceptionRecord->ExceptionAddress;
        auto code = info->ExceptionRecord->ExceptionCode;

        ExceptionInfo exceptionInfo {
            reinterpret_cast<uintptr_t>(addr),
            code, std::string(getExceptionCodeName(code))
        };

        s_crashHandler(CrashHandler(exceptionInfo));
    }

    void registerCrashHandler(std::function<void(CrashHandler const&)> handler) {
        s_crashHandler = std::move(handler);
        SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS info) -> LONG {
            switch (info->ExceptionRecord->ExceptionCode) {
                case DBG_EXCEPTION_HANDLED:
                case DBG_CONTINUE:
                case DBG_REPLY_LATER:
                case DBG_TERMINATE_THREAD:
                case DBG_TERMINATE_PROCESS:
                case DBG_RIPEXCEPTION:
                case DBG_CONTROL_BREAK:
                case DBG_COMMAND_EXCEPTION:
                case DBG_PRINTEXCEPTION_C:
                case DBG_PRINTEXCEPTION_WIDE_C:
                case DBG_CONTROL_C:
                case STATUS_CONTROL_C_EXIT:
                // case EXCEPTION_SET_THREAD_NAME:
                case RPC_S_SERVER_UNAVAILABLE:
                case RPC_S_CALL_FAILED:
                case RPC_S_CALL_FAILED_DNE:
                case RPC_S_PROTOCOL_ERROR:
                case static_cast<DWORD>(RPC_E_WRONG_THREAD):
                case RPC_S_SERVER_TOO_BUSY:
                case RPC_S_INVALID_STRING_BINDING:
                case RPC_S_WRONG_KIND_OF_BINDING:
                case RPC_S_INVALID_BINDING:
                    return EXCEPTION_CONTINUE_SEARCH;
                default:
                    handleCrash(info);
                    return EXCEPTION_CONTINUE_SEARCH;
            }
        });
    }

}
#endif