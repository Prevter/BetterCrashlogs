#pragma once

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

}