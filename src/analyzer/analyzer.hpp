#pragma once

#include <Windows.h>

namespace analyzer {

    struct ModuleInfo {
        HMODULE handle;
        std::string name;
        std::string path;
        uintptr_t baseAddress;
        uintptr_t size;

        bool contains(uintptr_t address) const {
            return address >= baseAddress && address < (baseAddress + size);
        }
    };

    /// @brief Analyze the exception information.
    void analyze(LPEXCEPTION_POINTERS info);

    /// @brief Cleanup the analyzer.
    void cleanup();

    /// @brief Reload the analyzer.
    void reload();

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

    /// @brief Get the value type of an address.
    ValueType getValueType(uintptr_t address);

    /// @brief Get decorated function name from an address.
    std::string getFunctionName(uintptr_t address);

    /// @brief Get the string from an address.
    std::string getString(uintptr_t address);

    /// @brief Get the string from a pointer.
    std::string getFromPointer(uintptr_t address);

    /// @brief Deduce the value from a pointer.
    std::pair<ValueType, std::string> getValue(uintptr_t address);

    /// @brief Get the exception message.
    /// @note This function should be called after the analyze function.
    /// @return The exception message that can be displayed to the user.
    const std::string &getExceptionMessage();

    /// @brief A struct containing the state of a register.
    struct RegisterState {
        std::string name; // The name of the register (e.g. EAX, EBX, etc.)
        uintptr_t value;   // The actual value of the register (e.g. 0x12345678)
        ValueType type;   // What type of value the register holds
        std::string description; // A description of the value (User-friendly value of the register)
    };

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
    const std::string& getRegisterStateMessage();

}