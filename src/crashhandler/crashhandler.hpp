#pragma once

#include <Geode/Geode.hpp>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <ranges>

#include "../utils/utils.hpp"

namespace breakdown {

class GeodeInfo {
public:
    class ModInfo {
    public:
        enum class Status {
            Disabled,
            Enabled,
            CurrentlyLoading,
            LoadProblems,
            OutdatedVersion,
            ShouldLoad
        };

        static constexpr char statusToChar(Status status) {
            switch (status) {
                case Status::Disabled: return ' ';
                case Status::Enabled: return 'x';
                case Status::CurrentlyLoading: return 'o';
                case Status::LoadProblems: return '!';
                case Status::OutdatedVersion: return '*';
                case Status::ShouldLoad: return '~';
            }
            return '?';
        }

        ModInfo(geode::Mod* mod);

        geode::Mod* getMod() const { return m_mod; }
        std::string_view getID() const { return m_id; }
        std::string_view getDisplayName() const { return m_displayName; }
        std::string_view getVersion() const { return m_version; }
        Status getStatus() const { return m_status; }

        std::string toString() const;

    private:
        geode::Mod* m_mod;
        std::string m_id;
        std::string m_displayName;
        std::string m_version;
        Status m_status;
    };

    GeodeInfo();

    std::filesystem::path const& getWorkingDirectory() const { return m_workingDirectory; }
    std::string_view getLoaderVersion() const { return m_loaderVersion; }
    std::string_view getGameVersion() const { return m_gameVersion; }
    std::string_view getLoaderCommit() const { return m_loaderCommit; }
    std::string_view getBindingsCommit() const { return m_bindingsCommit; }
    std::vector<ModInfo> const& getMods() const { return m_mods; }

    size_t getModCount() const { return m_mods.size(); }
    size_t getEnabledModCount() const;
    size_t getLoadedModCount() const;
    static size_t getOutdatedModCount();
    static size_t getProblemCount();

    std::string toString() const;
    std::string modListToString() const;

private:
    std::filesystem::path m_workingDirectory;
    std::string m_loaderVersion;
    std::string m_gameVersion;
    std::string_view m_loaderCommit;
    std::string_view m_bindingsCommit;
    std::vector<ModInfo> m_mods;
};

class ExceptionInfo {
public:
    ExceptionInfo(uintptr_t exceptionAddress, size_t exceptionCode, std::string exceptionName);

    std::string toString() const;

private:
    std::string m_threadName;
    std::thread::id m_threadID;
    uintptr_t m_exceptionAddress;
    size_t m_exceptionCode;
    std::string m_exceptionName;
};

class CrashHandler {
public:
    CrashHandler(ExceptionInfo exceptionInfo) : m_exceptionInfo(std::move(exceptionInfo)) {
        m_time = std::chrono::system_clock::now();
        m_wittyComment = utils::getWittyComment();
        m_geodeInfo = GeodeInfo();
        generateString();
        saveCrashlog();
    }

    std::chrono::time_point<std::chrono::system_clock> getTime() const { return m_time; }
    std::string getTimeString() const { return utils::formatTime(m_time); }

    std::string_view getWittyComment() const { return m_wittyComment; }
    GeodeInfo const& getGeodeInfo() const { return m_geodeInfo; }

    std::string const& getString() const { return m_string; }
    std::filesystem::path const& getCrashlogPath() const { return m_crashlogPath; }

private:
    void generateString();
    void saveCrashlog();
    std::string m_string; // the crashlog (generated once)
    std::filesystem::path m_crashlogPath;

    std::chrono::time_point<std::chrono::system_clock> m_time;
    std::string_view m_wittyComment;
    GeodeInfo m_geodeInfo;
    ExceptionInfo m_exceptionInfo;
};

inline thread_local CrashHandler* g_crashHandler = nullptr;

}