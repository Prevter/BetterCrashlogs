#include "crashhandler.hpp"

#include <ranges>

#include "../utils/utils.hpp"

// functions from Geode.dll, that are not exposed in the header
namespace about {
    const char* getLoaderVersionStr();
    const char* getLoaderCommitHash();
    const char* getBindingsCommitHash();
    const char* getLoaderModJson();
}

namespace breakdown {

    GeodeInfo::ModInfo::ModInfo(geode::Mod *mod) {
        m_mod = mod;
        m_id = mod->getID();
        m_displayName = mod->getName();
        m_version = mod->getVersion().toVString();
        m_status = mod->isCurrentlyLoading() ? Status::CurrentlyLoading :
                   mod->isEnabled() ? Status::Enabled :
                   mod->hasLoadProblems() ? Status::LoadProblems :
                   mod->targetsOutdatedVersion() ? Status::OutdatedVersion :
                   mod->shouldLoad() ? Status::ShouldLoad : Status::Disabled;
    }

    GeodeInfo::GeodeInfo() {
        m_workingDirectory = std::filesystem::current_path();
        auto loader = geode::Loader::get();
        m_loaderVersion = loader->getVersion().toVString();
        m_gameVersion = loader->getGameVersion();
        m_loaderCommit = about::getLoaderCommitHash();
        m_bindingsCommit = about::getBindingsCommitHash();
        m_mods = loader->getAllMods()
                 | std::views::transform([](geode::Mod* mod) { return ModInfo(mod); })
                 | std::ranges::to<std::vector<ModInfo>>();
    }

    std::string GeodeInfo::toString() const {
        return fmt::format(
            "- Working Directory: {}\n"
            "- Loader Version: {} (Geometry Dash v{})\n"
            "- Loader Commit: {}\n"
            "- Bindings Commit: {}\n"
            "- Installed Mods: {} (Loaded: {}/{})\n"
            "- Outdated Mods: {}\n"
            "- Problems: {}",
            m_workingDirectory.string(),
            m_loaderVersion, m_gameVersion,
            m_loaderCommit, m_bindingsCommit,
            getModCount(), getLoadedModCount(), getEnabledModCount(),
            getOutdatedModCount(),
            getProblemCount()
        );
    }

    std::string GeodeInfo::modListToString() const {
        std::string str;
        str.reserve(32 * m_mods.size());
        for (auto const& mod : m_mods) {
            str += fmt::format("{}\n", mod.toString());
        }
        str.pop_back(); // Remove trailing newline
        return str;
    }

    ExceptionInfo::ExceptionInfo(uintptr_t exceptionAddress, size_t exceptionCode, std::string exceptionName) {
        m_threadName = geode::utils::thread::getName();
        m_threadID = std::this_thread::get_id();
        m_exceptionAddress = exceptionAddress;
        m_exceptionCode = exceptionCode;
        m_exceptionName = std::move(exceptionName);
    }

    std::string ExceptionInfo::toString() const {
        return fmt::format(
            "- Thread Information: {} (ID: {})\n"
            "- Exception Code: {} (0x{:X})\n"
            "- Exception Address: 0x{:X}\n",
            m_threadName, m_threadID,
            m_exceptionName, m_exceptionCode,
            m_exceptionAddress
        );
    }

    void CrashHandler::generateString() {
        m_string = fmt::format(
            "{}\n{}\n\n"
            "== Geode Information ==\n{}\n\n"
            "== Exception Information ==\n{}\n\n"
            "== Installed Mods ==\n{}\n\n",
            getTimeString(), m_wittyComment,
            m_geodeInfo.toString(),
            m_exceptionInfo.toString(),
            m_geodeInfo.modListToString()
        );
    }

    void CrashHandler::saveCrashlog() {
        static auto crashlogsDir = geode::dirs::getCrashlogsDir();
        auto filename = fmt::format("{}.txt", utils::formatTime(m_time, true));
        m_crashlogPath = crashlogsDir / filename;

        std::filesystem::create_directories(crashlogsDir);
        std::ofstream file(m_crashlogPath);
        file << m_string;
        file.close();

        // Create empty "last-crashed" file to indicate that the game crashed
        std::ofstream lastCrashedFile(crashlogsDir / "last-crashed");
        lastCrashedFile.close();
    }
}
