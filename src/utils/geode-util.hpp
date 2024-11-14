#pragma once

#include <Geode/Geode.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#define GEODE_DLL __declspec(dllimport)

// Import the necessary functions from the Geode.dll
namespace about {
    const char *getLoaderVersionStr();
    const char *getLoaderCommitHash();
    const char *getBindingsCommitHash();
    const char *getLoaderModJson();
}

namespace utils::geode {

    /// @brief Returns a formatted string containing metadata about the loader.
    const std::string& getLoaderMetadataMessage();

    /// @brief Returns the version of the game.
    const std::string &getGameVersion();

    /// @brief Returns the version of the loader.
    std::string getLoaderVersion();

    /// @brief Get the amount of mods installed
    uint32_t getModCount();

    /// @brief Get the amount of mods loaded
    uint32_t getLoadedModCount();

    /// @brief Get the amount of mods enabled
    uint32_t getEnabledModCount();

    /// @brief Get the path to the crashlogs folder
    std::filesystem::path getCrashlogsPath();

    /// @brief Get the path to the mod resources folder
    std::filesystem::path getResourcesPath();

    /// @brief Get the path to the mod config folder
    std::filesystem::path getConfigPath();

    enum class ModStatus {
        Disabled, // ' '
        IsCurrentlyLoading, // 'o'
        Enabled, // 'x'
        HasProblems, // '!'
        ShouldLoad, // '~',
        Outdated, // '*'
    };

    struct ModInfo {
        std::string name;
        std::string id;
        std::string version;
        std::string developer;
        ModStatus status;
        ::geode::Mod* mod;
    };

    /// @brief Get installed mods list
    const std::vector<ModInfo>& getModList();

    /// @brief Get installed/loaded mods list message
    const std::string& getModListMessage();

    const std::unordered_map<uintptr_t, std::string>& getFunctionAddresses();
    const std::unordered_map<uintptr_t, std::string>& getCocosFunctionAddresses();

    /// @brief Try to find the function address and name from the given address.
    /// @note If the address is not found, the name will be an empty string
    /// @param address The address to search for
    /// @param moduleBase The base address of the module
    /// @param useCocos Whether to compare against libcocos2d symbols
    std::pair<uintptr_t, std::string> getFunctionAddress(uintptr_t address, uintptr_t moduleBase, bool useCocos = false);

    /// @brief Check whether current system is running Wine.
    bool isWine();

    /// @brief Check whether intrusive mode is enabled.
    bool intrusiveEnabled();

    inline std::string formatFileURL(const std::string& file) {
        return fmt::format("https://prevter.github.io/bindings-meta/{}", file);
    }

    inline std::string getCocosFile() {
        return fmt::format(
            "libcocos2d-{}.txt",
            utils::geode::getGameVersion()
        );
    }

    inline std::string getBindingsFile() {
        return fmt::format(
            "{}-{}{}.txt",
            GEODE_MACOS("MacOS")
            GEODE_WINDOWS64("Win64")
            GEODE_WINDOWS32("Win32"),
            utils::geode::getGameVersion(),
            GEODE_WINDOWS("")
            GEODE_ARM_MAC("-Arm")
            GEODE_INTEL_MAC("-Intel")
        );
    }
}