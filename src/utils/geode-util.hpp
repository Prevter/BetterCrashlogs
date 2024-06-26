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
        ShouldLoad, // '~'
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

    /// @brief Try to find the function address and name from the given address.
    /// @note If the address is not found, the name will be an empty string
    /// @param address The address to search for
    /// @param moduleBase The base address of the module
    std::pair<uintptr_t, std::string> getFunctionAddress(uintptr_t address, uintptr_t moduleBase);

    /// @brief Check whether current system is running Wine.
    bool isWine();
}