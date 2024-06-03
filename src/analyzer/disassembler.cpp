#include "disassembler.hpp"

#include <zydis/zydis.h>
#include <fmt/format.h>
#include <sstream>
#include <unordered_map>

#ifdef _WIN32
#ifdef _WIN64
#define TARGET_ARCH ZYDIS_MACHINE_MODE_LONG_64
#define TARGET_ADDR_WIDTH ZYDIS_STACK_WIDTH_64
#else
#define TARGET_ARCH ZYDIS_MACHINE_MODE_LONG_COMPAT_32
#define TARGET_ADDR_WIDTH ZYDIS_STACK_WIDTH_32
#endif
#else
#define TARGET_ARCH ZYDIS_MACHINE_MODE_LONG_64
#define TARGET_ADDR_WIDTH ZYDIS_STACK_WIDTH_64
#endif

namespace disasm {

    static ZydisDecoder decoder;
    static ZydisFormatter formatter;

    std::unordered_map<uintptr_t, Instruction> cache;

    const Instruction& disassemble(uintptr_t address) {
        if (cache.find(address) != cache.end()) {
            return cache[address];
        }

        Instruction instruction;
        instruction.address = address;

        ZyanUSize offset = 0;
        ZydisDisassembledInstruction ins;
        while (ZYAN_SUCCESS(ZydisDisassembleIntel(
                /* machine_mode:    */ TARGET_ARCH,
                /* runtime_address: */ address,
                /* buffer:          */ reinterpret_cast<const void *>(address + offset),
                /* length:          */ 16 - offset,
                /* instruction:     */ &ins
        ))) {
            instruction.text = ins.text;
            instruction.size = ins.info.length;

            // Get the bytes
            std::vector<uint8_t> bytes(ins.info.length);
            for (int i = 0; i < ins.info.length; i++) {
                bytes[i] = *(reinterpret_cast<uint8_t *>(address + offset + i));
            }

            // Convert to hex string
            std::stringstream ss;
            for (const auto& byte : bytes) {
                ss << fmt::format("{:02X} ", byte);
            }
            instruction.bytes = ss.str().substr(0, ss.str().size() - 1);
            break;
        }

        cache[address] = instruction;
        return cache[address];
    }

    std::vector<Instruction> disassemble(uintptr_t start, uintptr_t end) {
        std::vector<Instruction> instructions;
        for (uintptr_t i = start; i <= end;) {
            auto ins = disassemble(i);
            instructions.push_back(ins);
            i += ins.size;
        }
        return instructions;
    }

}