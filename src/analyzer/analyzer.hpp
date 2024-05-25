#pragma once

#include <Windows.h>

#include <utility>

namespace analyzer {

    struct ModuleInfo {
        HMODULE handle;
        std::string name;
        std::string path;
        uintptr_t baseAddress;
        uintptr_t size;

        [[nodiscard]] bool contains(void *address) const {
            return address >= (void *) baseAddress && address < (void *) (baseAddress + size);
        }
    };

    enum class ValueType {
        // Unknown type
        Unknown,
        // A valid pointer to a memory address
        Pointer,
        // A valid function pointer
        Function,
        // A valid string pointer
        String,
    };

    struct MethodInfo {
        std::string module; // Module name
        uintptr_t address; // Address, relative to the module
        std::string name; // Function name
        uintptr_t offset; // Offset from the function start

        std::string file; // File name (if available)
        uint32_t line; // Line number (if available)

        /// @brief Default constructor.
        MethodInfo() : address(0), offset(0), line(0) {}

        /// @brief No-module constructor.
        MethodInfo(uintptr_t addr, uintptr_t offset)
                : address(addr), offset(offset), line(0) {}

        /// @brief Constructor with module.
        MethodInfo(std::string mod, uintptr_t addr, uintptr_t offset)
                : module(std::move(mod)), address(addr), offset(offset), line(0) {}

        /// @brief Constructor with module and function name.
        MethodInfo(std::string mod, uintptr_t addr, std::string nm, uintptr_t offset)
                : module(std::move(mod)), address(addr), name(std::move(nm)), offset(offset), line(0) {}

        /// @brief Convert the method info to a string.
        [[nodiscard]] std::string toString() const;
    };

    /// @brief A struct containing the state of a register.
    struct RegisterState {
        std::string name; // The name of the register (e.g. EAX, EBX, etc.)
        uintptr_t value;   // The actual value of the register (e.g. 0x12345678)
        ValueType type;   // What type of value the register holds
        std::string description; // A description of the value (User-friendly value of the register)
    };

    struct StackLine {
        uintptr_t address;
        uintptr_t value;
        ValueType type;
        std::string description;
    };

    struct StackTraceLine {
        uintptr_t address{}; // Address of the function (absolute)
        ModuleInfo module; // Module information
        uintptr_t moduleOffset{}; // Offset from the module base
        MethodInfo function; // Function information
        uintptr_t framePointer{}; // Stack frame pointer
    };

    class Analyzer {
    private:
        LPEXCEPTION_POINTERS exceptionInfo = nullptr;
        std::string threadInfo;
        std::vector<ModuleInfo> modules;
        bool debugSymbolsLoaded = false;

        std::string exceptionMessage;
        std::vector<RegisterState> registerStates;
        std::map<std::string, bool> cpuFlags;
        std::vector<StackLine> stackData;
        std::string stackAllocationsMessage;
        std::vector<StackTraceLine> stackTrace;
        std::string stackTraceMessage;
        std::string registerStateMessage;
        bool mainThreadCrash = false;
    public:

        /// @brief Analyze the exception information.
        void analyze(LPEXCEPTION_POINTERS info);

        /// @brief Cleanup the analyzer.
        void cleanup();

        /// @brief Reload the analyzer.
        void reload();

        /// @brief Get the value type of an address.
        static ValueType getValueType(uintptr_t address);

        /// @brief Get the function information from an address.
        static MethodInfo getFunction(uintptr_t address);

        /// @brief Get the string from an address.
        static std::string getString(uintptr_t address);

        /// @brief Get the string from a pointer.
        std::string getFromPointer(uintptr_t address, size_t depth = 0);

        /// @brief Deduce the value from a pointer.
        std::pair<ValueType, std::string> getValue(uintptr_t address);

        /// @brief Get the exception message.
        /// @note This function should be called after the analyze function.
        /// @return The exception message that can be displayed to the user.
        const std::string &getExceptionMessage();

        /// @brief Constructs a register state.
        RegisterState setupRegisterState(const std::string &name, uintptr_t value);

        /// @brief Get register states.
        /// @note This function should be called after the analyze function.
        /// @return The register states that can be displayed to the user.
        const std::vector<RegisterState> &getRegisterStates();

        /// @brief Get the CPU flags.
        /// @note This function should be called after the analyze function.
        /// @return The CPU flags that can be displayed to the user.
        const std::map<std::string, bool> &getCpuFlags();

        /// @brief Get the register state message.
        /// @note This function should be called after the analyze function.
        /// @return The register state message that can be displayed to the user.
        const std::string &getRegisterStateMessage();

        /// @brief Get the stack allocated data. (Latest 32 entries)
        /// @note This function should be called after the analyze function.
        /// @return The stack data that can be displayed to the user.
        const std::vector<StackLine> &getStackData();

        /// @brief Get the information about the stack allocations.
        const std::string &getStackAllocationsMessage();

        /// @brief Get the stack trace.
        /// @note This function should be called after the analyze function.
        /// @return The stack trace that can be displayed to the user.
        const std::vector<StackTraceLine> &getStackTrace();

        /// @brief Get the stack trace message.
        /// @note This function should be called after the analyze function.
        /// @return The stack trace message that can be displayed to the user.
        const std::string &getStackTraceMessage();

        /// @brief Check if the graphics driver crashed. (stack trace contains GPU driver dll)
        bool isGraphicsDriverCrash();

        /// @brief Get the module information from an address.
        ModuleInfo *getModuleInfo(void *address);

        /// @brief Check whether the crash happened in the main thread
        bool isMainThread();
    };
}