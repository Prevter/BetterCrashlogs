#pragma once

#define GEODE_DLL __declspec(dllimport)

// Import the necessary functions from the Geode.dll
namespace about {
    GEODE_DLL const char *getLoaderVersionStr();

    GEODE_DLL const char *getLoaderCommitHash();

    GEODE_DLL const char *getBindingsCommitHash();

    GEODE_DLL const char *getLoaderModJson();
}

namespace utils::geode {

    /// @brief Returns a formatted string containing metadata about the loader.
    const std::string& getLoaderMetadataMessage();

    /// @brief Returns the version of the loader.
    std::string getLoaderVersion();

    /// @brief Get the amount of mods installed
    uint32_t getModCount();

    /// @brief Get the amount of mods loaded
    uint32_t getLoadedModCount();

    /// @brief Get the path to the crashlogs folder
    ghc::filesystem::path getCrashlogsPath();

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

}