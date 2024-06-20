#pragma once

#include <string>
#include <cstdint>

/// @brief Hardware information utilities.
namespace hwinfo {

    namespace ram {
        /// @brief Get the amount of RAM installed.
        /// @return Amount of RAM installed in megabytes.
        uint64_t total();

        /// @brief Get the amount of RAM used.
        /// @return Amount of RAM used in megabytes.
        uint64_t used();

        /// @brief Get the amount of RAM free.
        /// @return Amount of RAM free in megabytes.
        uint64_t free();
    }

    namespace swap {
        /// @brief Get the amount of swap space available.
        /// @return Amount of swap space available in megabytes.
        uint64_t total();

        /// @brief Get the amount of swap space used.
        /// @return Amount of swap space used in megabytes.
        uint64_t used();

        /// @brief Get the amount of swap space free.
        /// @return Amount of swap space free in megabytes.
        uint64_t free();
    }

    /// @brief Get the name of the CPU.
    /// @return Name of the CPU (e.g. "AMD Ryzen 5 2600 Six-Core Processor").
    std::string getCPUName();

    /// @brief Get the name of the GPU.
    /// @return Name of the GPU (e.g. "NVIDIA GeForce GTX 1660 Ti").
    std::string getGPUName();

    /// @brief Get the amount of CPU cores.
    /// @return Amount of CPU cores.
    uint32_t getCPUCores();

    /// @brief Get the amount of CPU threads.
    /// @return Amount of CPU threads.
    uint32_t getCPUThreads();

    /// @brief Get the name of the operating system.
    /// @return Name of the operating system (e.g. "Windows 11 x64 (v.10.0.22000.318)").
    std::string getOSName();

    /// @brief Get the message containing all hardware information.
    std::string getMessage();

}