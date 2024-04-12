#include "analyzer.hpp"

#include "exception-codes.hpp"
#include "../utils/memory.hpp"
#include "../utils/utils.hpp"

#pragma comment(lib, "DbgHelp")

namespace analyzer {

    static LPEXCEPTION_POINTERS exceptionInfo = nullptr;
    static std::string threadInfo;
    static std::vector<ModuleInfo> modules;
    static bool debugSymbolsLoaded = false;

    static std::string exceptionMessage;
    static std::vector<RegisterState> registerStates;
    static std::map<std::string, bool> cpuFlags;
    static std::vector<StackLine> stackData;

    void analyze(LPEXCEPTION_POINTERS info) {
        exceptionInfo = info;

        // Get all module handles
        if (modules.empty()) {
            DWORD numModules;
            HMODULE moduleHandles[1024];
            if (EnumProcessModules(GetCurrentProcess(), moduleHandles, sizeof(moduleHandles), &numModules)) {
                for (DWORD i = 0; i < numModules / sizeof(HMODULE); i++) {
                    char buffer[MAX_PATH];
                    GetModuleFileNameA(moduleHandles[i], buffer, MAX_PATH);
                    MODULEINFO moduleInfo;
                    GetModuleInformation(GetCurrentProcess(), moduleHandles[i], &moduleInfo, sizeof(moduleInfo));
                    modules.push_back(
                            {moduleHandles[i], utils::getFileName(buffer), buffer, (uintptr_t) moduleInfo.lpBaseOfDll,
                             (uintptr_t) moduleInfo.SizeOfImage});
                }
            }
        }

        // Load debug symbols
        if (!debugSymbolsLoaded) {
            SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
            debugSymbolsLoaded = SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        }

        // Get thread ID and name
        if (threadInfo.empty()) {
            auto threadId = GetCurrentThreadId();
            auto threadHandle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadId);
            if (threadHandle != nullptr) {
                wchar_t *threadName = nullptr;
                if (GetThreadDescription(threadHandle, &threadName) != 0) {
                    std::wstring threadNameWStr(threadName);
                    std::string threadNameStr(threadNameWStr.begin(), threadNameWStr.end());
                    threadInfo = fmt::format("\"{}\" (ID: {})", threadNameStr, threadId);
                } else {
                    threadInfo = fmt::format("(ID: {})", threadId);
                }
            } else {
                threadInfo = fmt::format("(ID: {})", threadId);
            }
        }
    }

    void cleanup() {
        // Unload debug symbols
        if (debugSymbolsLoaded) {
            SymCleanup(GetCurrentProcess());
        }

        // Reset all data
        debugSymbolsLoaded = false;
        exceptionMessage.clear();
        registerStates.clear();
        cpuFlags.clear();
        stackData.clear();
    }

    void reload() {
        cleanup();
        analyze(exceptionInfo);
    }

    const std::string &getExceptionMessage() {
        if (!exceptionMessage.empty())
            return exceptionMessage;

        auto exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
        auto exceptionName = exceptions::getName(exceptionCode);
        auto extraInfo = exceptions::getExtraInfo(exceptionCode, exceptionInfo);
        auto parameters = exceptions::getParameters(exceptionCode, exceptionInfo);

        exceptionMessage = fmt::format(
                "- Thread Information: {}\n"
                "- Exception Code: {} (0x{:X})\n"
                "- Exception Address: 0x{:X} ({})\n"
                "- Exception Flags: 0x{:X}\n"
                "- Exception Parameters: {}{}",
                threadInfo,
                exceptionName, exceptionCode,
                (uintptr_t) exceptionInfo->ExceptionRecord->ExceptionAddress,
                getFunctionName((uintptr_t) exceptionInfo->ExceptionRecord->ExceptionAddress),
                exceptionInfo->ExceptionRecord->ExceptionFlags,
                parameters,
                extraInfo.empty() ? "" : fmt::format("\n{}", extraInfo)
        );

        return exceptionMessage;
    }

    ValueType getValueType(uintptr_t address) {
        if (!utils::mem::isAccessible(address))
            return ValueType::Unknown;

        if (utils::mem::isStringPtr(address))
            return ValueType::String;

        if (utils::mem::isFunctionPtr(address))
            return ValueType::Function;

        return ValueType::Pointer;
    }

    std::string getFunctionName(uintptr_t address) {
        HMODULE module = utils::mem::getModuleHandle(address);
        if (module == nullptr)
            return fmt::format("0x{:X}", address);

        auto moduleName = utils::mem::getModuleName(module);
        auto moduleOffset = (uintptr_t) address - reinterpret_cast<DWORD>(module);

        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCTSTR) address, &module)) {
            static char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            auto pSymbol = (PSYMBOL_INFO) buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement;
            if (SymFromAddr(GetCurrentProcess(), (DWORD64) address, &displacement, pSymbol)) {
                return fmt::format("{} -> {}+0x{:X}", moduleName, pSymbol->Name, displacement);
            }
        }

//        // Check if it's the main module
//        if (module == GetModuleHandle(nullptr)) {
//            auto gdName = getGeometryDashMethodName((uintptr_t) address);
//            if (!gdName.empty()) {
//                return fmt::format("{} -> {}", moduleName, gdName);
//            }
//        }

        return fmt::format("{}+0x{:X}", moduleName, moduleOffset);
    }

    std::string getString(uintptr_t address) {
        const char *str = (const char *) address;
        return fmt::format("&\"{}\"", str);
    }

    std::string getFromPointer(uintptr_t address) {
        uintptr_t value = *(uintptr_t *) address;
        return fmt::format("-> 0x{:X}", value);
    }

    std::pair<ValueType, std::string> getValue(uintptr_t address) {
        switch (getValueType(address)) {
            case ValueType::Function:
                return {ValueType::Function, getFunctionName(address)};
            case ValueType::String:
                return {ValueType::String, getString(address)};
            case ValueType::Pointer:
                return {ValueType::Pointer, getFromPointer(address)};
            default:
                return {ValueType::Unknown, fmt::format("{}i | {}u", address, *(uint32_t *) &address)};
        }
    }

    inline RegisterState setupRegisterState(const std::string &name, uintptr_t value) {
        auto valueResult = getValue(value);
        return {name, value, valueResult.first, valueResult.second};
    }

    const std::vector<RegisterState> &getRegisterStates() {
        if (!registerStates.empty())
            return registerStates;

        CONTEXT context = *exceptionInfo->ContextRecord;
        registerStates = {
                setupRegisterState("EAX", context.Eax),
                setupRegisterState("EBX", context.Ebx),
                setupRegisterState("ECX", context.Ecx),
                setupRegisterState("EDX", context.Edx),
                setupRegisterState("ESI", context.Esi),
                setupRegisterState("EDI", context.Edi),
                setupRegisterState("EBP", context.Ebp),
                setupRegisterState("ESP", context.Esp),

                setupRegisterState("EIP", context.Eip),
        };

        return registerStates;
    }

    const std::string &getRegisterStateMessage() {
        static std::string registerStateMessage;
        if (!registerStateMessage.empty())
            return registerStateMessage;

        auto registers = getRegisterStates();
        for (const auto &reg: registers) {
            registerStateMessage += fmt::format(
                    "- {}: {:08X} ({})\n",
                    reg.name, reg.value, reg.description
            );
        }

        // fix 3 flags per line
        auto flags = getCpuFlags();
        int i = 0;
        for (const auto &flag: flags) {
            if (i % 3 == 0) {
                registerStateMessage += "- ";
            }
            registerStateMessage += fmt::format("{}: {} | ", flag.first, flag.second ? "1" : "0");
            if (i % 3 == 2) {
                registerStateMessage.pop_back();
                registerStateMessage.pop_back();
                registerStateMessage += "\n";
            }
            i++;
        }

        if (!registerStateMessage.empty()) {
            registerStateMessage.pop_back();
        }

        return registerStateMessage;
    }

    const std::map<std::string, bool> &getCpuFlags() {
        if (!cpuFlags.empty())
            return cpuFlags;

        CONTEXT context = *exceptionInfo->ContextRecord;
        cpuFlags = {
                {"CF", context.EFlags & 0x0001},
                {"PF", context.EFlags & 0x0004},
                {"AF", context.EFlags & 0x0010},
                {"ZF", context.EFlags & 0x0040},
                {"SF", context.EFlags & 0x0080},
                {"TF", context.EFlags & 0x0100},
                {"IF", context.EFlags & 0x0200},
                {"DF", context.EFlags & 0x0400},
                {"OF", context.EFlags & 0x0800},
        };

        return cpuFlags;
    }

    const std::vector<StackLine>& getStackData() {
        if (!stackData.empty())
            return stackData;

        CONTEXT context = *exceptionInfo->ContextRecord;
        uintptr_t stackPointer = context.Esp;
        for (int i = 0; i < 20; i++) {
            uintptr_t address = stackPointer + i * sizeof(uintptr_t);
            uintptr_t value = *(uintptr_t *) address;
            auto valueResult = getValue(value);
            stackData.push_back({address, value, valueResult.first, valueResult.second});
        }

        return stackData;
    }

}