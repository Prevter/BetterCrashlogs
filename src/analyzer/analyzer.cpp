#include "analyzer.hpp"

#include "exception-codes.hpp"
#include "../utils/memory.hpp"
#include "../utils/utils.hpp"
#include "../utils/geode-util.hpp"

#pragma comment(lib, "dbghelp")

namespace analyzer {

    static HANDLE s_mainThread = GetCurrentThread();

    void Analyzer::analyze(LPEXCEPTION_POINTERS info) {
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

                    // Check if the crash happened on the main thread
                    mainThreadCrash = threadNameStr == "Main";
                } else {
                    threadInfo = fmt::format("(ID: {})", threadId);
                }
            } else {
                threadInfo = fmt::format("(ID: {})", threadId);
            }
        }
    }

    void Analyzer::cleanup() {
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
        stackAllocationsMessage.clear();
        stackTrace.clear();
        stackTraceMessage.clear();
    }

    void Analyzer::reload() {
        cleanup();
        analyze(exceptionInfo);
    }

    uintptr_t getThreadStartAddress(HANDLE thread) {
        // use ThreadQuerySetWin32StartAddress to get the start address of the thread
        typedef NTSTATUS(NTAPI *pNtQueryInformationThread)(HANDLE, LONG, PVOID, ULONG, PULONG);
        static auto NtQueryInformationThread = (pNtQueryInformationThread) GetProcAddress(
                GetModuleHandleA("ntdll.dll"),
                "NtQueryInformationThread");
        if (!NtQueryInformationThread) return 0;

        // get the thread start address
        PVOID threadStartAddress;
        NtQueryInformationThread(thread, 9, &threadStartAddress, sizeof(threadStartAddress), nullptr);
        return (uintptr_t) threadStartAddress;
    }

    const std::string &Analyzer::getExceptionMessage() {
        if (!exceptionMessage.empty())
            return exceptionMessage;

        auto exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
        auto exceptionName = exceptions::getName(exceptionCode);
        auto extraInfo = exceptions::getExtraInfo(exceptionCode, exceptionInfo);
        auto parameters = exceptions::getParameters(exceptionCode, exceptionInfo);

        // get thread start address
        auto threadStartAddress = getThreadStartAddress(GetCurrentThread());
        auto threadStartFunction = getFunction((uintptr_t) threadStartAddress);

        exceptionMessage = fmt::format(
                "- Thread Information: {}\n"
                "- Thread Start Address: {}\n"
                "- Exception Code: {} (0x{:X})\n"
                "- Exception Address: {}\n"
                "- Exception Flags: 0x{:X}\n"
                "- Exception Parameters: {}{}",
                threadInfo, threadStartFunction.toString(),
                exceptionName, exceptionCode,
                getFunction((uintptr_t) exceptionInfo->ExceptionRecord->ExceptionAddress).toString(),
                exceptionInfo->ExceptionRecord->ExceptionFlags,
                parameters,
                extraInfo.empty() ? "" : fmt::format("\n{}", extraInfo)
        );

        return exceptionMessage;
    }

    ValueType Analyzer::getValueType(uintptr_t address) {
        if (!utils::mem::isAccessible(address))
            return ValueType::Unknown;

        if (utils::mem::isStringPtr(address))
            return ValueType::String;

        if (utils::mem::isFunctionPtr(address))
            return ValueType::Function;

        return ValueType::Pointer;
    }

    std::string MethodInfo::toString() const {
        if (address == 0) {
            return fmt::format("0x{:08X}", offset); // Unknown function
        }

        if (module.empty()) {
            return fmt::format("0x{:08X}+0x{:X}", address, offset); // No module name
        }

        if (name.empty()) {
            return fmt::format("{}+0x{:X}", module, address); // No function name
        }

        return fmt::format("{}+0x{:X} ({}+0x{:x})", module, address, name, offset);
    }

    MethodInfo Analyzer::getFunction(uintptr_t address) {
        HMODULE module = utils::mem::getModuleHandle(address);
        if (module == nullptr) {
            return {0, address};
        }

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
                return {moduleName, moduleOffset, pSymbol->Name, static_cast<uintptr_t>(displacement)};
            }
        }

        // Check if it's the main module
        if (module == GetModuleHandle(nullptr)) {
            // Look up the function name in the CodegenData.txt file
            auto methodInfo = utils::geode::getFunctionAddress(address, (uintptr_t) module);
            if (methodInfo.first != 0) {
                auto methodOffset = moduleOffset - methodInfo.first;
                auto methodName = methodInfo.second;
                if (methodName.empty()) {
                    return {moduleName, moduleOffset, fmt::format("<0x{:x}>", methodInfo.first), methodOffset};
                }
                return {moduleName, moduleOffset, methodName, methodOffset};
            }

            return {moduleName, moduleOffset, 0};
        }

        {
            auto methodStart = utils::mem::findMethodStart(address);
            if (methodStart == 0) {
                return {moduleName, moduleOffset, 0};
            }
            auto methodOffset = (uintptr_t) address - methodStart;
            methodStart -= (uintptr_t) module; // Get the offset from the module base
            return {moduleName, moduleOffset, fmt::format("<0x{:x}>", methodStart), methodOffset};
        }
    }

    std::string Analyzer::getString(uintptr_t address) {
        const char *str = (const char *) address;
        return fmt::format("&\"{}\"", str);
    }

    std::string Analyzer::getFromPointer(uintptr_t address, size_t depth) {
        uintptr_t value = *(uintptr_t *) address;

        if (depth > 10) { // Prevent infinite recursion
            return fmt::format("-> 0x{:X} [...]", value);
        }

        // Check if this was a pointer to a pointer
        auto valueType = getValueType(value);
        switch (valueType) {
            case ValueType::Function:
                return fmt::format("-> 0x{:X} -> {}", value, getFunction(value).toString());
            case ValueType::String:
                return fmt::format("-> 0x{:X} -> {}", value, getString(value));
            case ValueType::Pointer:
                return fmt::format("-> 0x{:X} {}", value, getFromPointer(value, depth + 1));
            default:
                return fmt::format("-> 0x{:X}", value);
        }
    }

    std::pair<ValueType, std::string> Analyzer::getValue(uintptr_t address) {
        switch (getValueType(address)) {
            case ValueType::Function:
                return {ValueType::Function, getFunction(address).toString()};
            case ValueType::String:
                return {ValueType::String, getString(address)};
            case ValueType::Pointer:
                return {ValueType::Pointer, getFromPointer(address)};
            default:
                return {ValueType::Unknown, fmt::format("{}i | {}u", address, *(uint32_t *) &address)};
        }
    }

    RegisterState Analyzer::setupRegisterState(const std::string &name, uintptr_t value) {
        auto valueResult = getValue(value);
        return {name, value, valueResult.first, valueResult.second};
    }

    const std::vector<RegisterState> &Analyzer::getRegisterStates() {
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

    const std::string &Analyzer::getRegisterStateMessage() {
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

    const std::map<std::string, bool> &Analyzer::getCpuFlags() {
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

    const std::vector<StackLine> &Analyzer::getStackData() {
        if (!stackData.empty())
            return stackData;

        CONTEXT context = *exceptionInfo->ContextRecord;
        uintptr_t stackPointer = context.Esp;
        for (int i = 0; i < 32; i++) {
            uintptr_t address = stackPointer + i * sizeof(uintptr_t);
            uintptr_t value = *(uintptr_t *) address;
            auto valueResult = getValue(value);
            stackData.push_back({address, value, valueResult.first, valueResult.second});
        }

        return stackData;
    }

    const std::string &Analyzer::getStackAllocationsMessage() {
        if (!stackAllocationsMessage.empty())
            return stackAllocationsMessage;

        auto data = getStackData();
        for (const auto &stackLine: data) {
            stackAllocationsMessage += fmt::format(
                    "- 0x{:X}: {:08X} ({})\n",
                    stackLine.address, stackLine.value, stackLine.description
            );
        }

        if (!stackAllocationsMessage.empty()) {
            stackAllocationsMessage.pop_back();
        }

        return stackAllocationsMessage;
    }

    ModuleInfo *Analyzer::getModuleInfo(void *address) {
        for (auto &module: modules) {
            if (module.contains(address)) {
                return &module;
            }
        }
        return nullptr;
    }

    const std::vector<StackTraceLine> &Analyzer::getStackTrace() {
        if (!stackTrace.empty())
            return stackTrace;

        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(STACKFRAME64));

        auto ctx = exceptionInfo->ContextRecord;

        DWORD machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = ctx->Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = ctx->Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = ctx->Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        while (StackWalk64(machineType, process, thread, &stackFrame, ctx, nullptr, SymFunctionTableAccess64,
                           SymGetModuleBase64, nullptr)) {
            if (stackFrame.AddrPC.Offset == 0) {
                break;
            }
            StackTraceLine line{};
            line.address = stackFrame.AddrPC.Offset;
            line.function = getFunction(stackFrame.AddrPC.Offset);
            line.framePointer = stackFrame.AddrFrame.Offset;
            auto module = getModuleInfo((void *) stackFrame.AddrPC.Offset);
            if (module) {
                line.module = *module;
                line.moduleOffset = stackFrame.AddrPC.Offset - (uintptr_t) module->handle;
            }

            if (debugSymbolsLoaded) {
                DWORD displacement;
                IMAGEHLP_LINE64 lineInfo;
                lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &displacement, &lineInfo)) {
                    line.function.file = lineInfo.FileName;
                    line.function.line = lineInfo.LineNumber;
                }
            }

            stackTrace.push_back(line);
        }

        return stackTrace;
    }

    const std::string &Analyzer::getStackTraceMessage() {
        if (!stackTraceMessage.empty())
            return stackTraceMessage;

        auto data = getStackTrace();

        for (const auto &stackLine: data) {
            if (stackLine.function.module.empty()) {    // Likely a virtual function
                if (stackLine.function.address == 0) {  // Function start not found
                    stackTraceMessage += fmt::format("- 0x{:08X}\n", stackLine.function.offset);
                } else {
                    stackTraceMessage += fmt::format("- 0x{:08X}+0x{:x}\n", stackLine.function.address,
                                                     stackLine.function.offset);
                }
                continue;
            }

            if (stackLine.function.name.empty()) {
                stackTraceMessage += fmt::format("- {}+0x{:X}\n", stackLine.function.module, stackLine.moduleOffset);
            } else {
                stackTraceMessage += fmt::format(
                        "- {}+0x{:X} ({}+0x{:x})\n",
                        stackLine.function.module, stackLine.function.address,
                        stackLine.function.name, stackLine.function.offset);
            }

            if (!stackLine.function.file.empty()) {
                stackTraceMessage += fmt::format("└ {}:{}\n", stackLine.function.file, stackLine.function.line);
            }
        }

        if (!stackTraceMessage.empty()) {
            stackTraceMessage.pop_back();
        }

        return stackTraceMessage;
    }

    bool Analyzer::isGraphicsDriverCrash() {
        // check latest 3 stack frames
        auto trace = getStackTrace();
        for (int i = 0; i < 3 && i < trace.size(); i++) {
            auto &line = trace[i];
            if (line.module.name.find("nvoglv32.dll") != std::string::npos || // NVIDIA
                line.module.name.find("atioglxx.dll") != std::string::npos || // AMD
                line.module.name.find("ig9icd32.dll") != std::string::npos) { // Intel
                return true;
            }
        }
        return false;
    }

    bool Analyzer::isMainThread() {
        return mainThreadCrash;
    }

}