#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace disasm {

    struct Instruction {
        std::string bytes;
        std::string text;
        uintptr_t address;
        size_t size;

        Instruction() : address(0) {}

        [[nodiscard]] std::string toString() const {
            return fmt::format("{:08X} | {} | {}", address, bytes, text);
        }
    };

    /// @brief Get an instruction from a given address.
    /// @param address The address to disassemble.
    /// @return The disassembled instruction.
    /// @note The result is cached.
    const Instruction& disassemble(uintptr_t address);

    /// @brief Get the disassembled instructions from a given address.
    /// @param start The start address.
    /// @param end The end address.
    /// @return The disassembled instructions.
    std::vector<Instruction> disassemble(uintptr_t start, uintptr_t end);

}