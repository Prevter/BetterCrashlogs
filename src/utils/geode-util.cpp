#include "geode-util.hpp"
#include <Geode/Geode.hpp>

namespace utils::geode {

    const char *problemTypeToString(::geode::LoadProblem::Type type) {
#define CASE(x) case ::geode::LoadProblem::Type::x: return #x
        switch (type) {
            CASE(Unknown);
            CASE(Suggestion);
            CASE(Recommendation);
            CASE(Conflict);
            CASE(OutdatedConflict);
            CASE(InvalidFile);
            CASE(Duplicate);
            CASE(SetupFailed);
            CASE(LoadFailed);
            CASE(EnableFailed);
            CASE(MissingDependency);
            CASE(PresentIncompatibility);
            CASE(UnzipFailed);
            CASE(UnsupportedVersion);
            CASE(UnsupportedGeodeVersion);
            CASE(NeedsNewerGeodeVersion);
            CASE(DisabledDependency);
            CASE(OutdatedDependency);
            CASE(OutdatedIncompatibility);
        }
#undef CASE
    }

    std::string readProblems(const std::vector<::geode::LoadProblem> &problems) {
        std::string result;

        for (auto &problem: problems) {
            std::variant<ghc::filesystem::path, ::geode::ModMetadata, ::geode::Mod *> cause = problem.cause;
            auto type = problemTypeToString(problem.type);
            std::string message = problem.message;
            if (std::holds_alternative<ghc::filesystem::path>(cause)) {
                auto path = std::get<ghc::filesystem::path>(cause);
                result += fmt::format("Type: {}\n"
                                      "Path: {}\n"
                                      "Message: {}\n\n",
                                      type, path.string(), message);
            } else if (std::holds_alternative<::geode::ModMetadata>(cause)) {
                auto mod = std::get<::geode::ModMetadata>(cause);
                result += fmt::format("Type: {}\n"
                                      "Mod: {}\n"
                                      "Message: {}\n\n",
                                      type, mod.getID(), message);
            } else if (std::holds_alternative<::geode::Mod *>(cause)) {
                auto mod = std::get<::geode::Mod *>(cause);
                result += fmt::format("Type: {}\n"
                                      "Mod: {}\n"
                                      "Message: {}\n\n",
                                      type, mod->getID(), message);
            }
        }

        if (!result.empty()) {
            result.pop_back();
            result.pop_back();
        }

        return result;
    }

    const std::string &getLoaderMetadataMessage() {
        static std::string message;
        if (!message.empty()) {
            return message;
        }

        char wd[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, wd);

        auto problems = ::geode::Loader::get()->getProblems();

        message = fmt::format(
                "- Working Directory: {}\n"
                "- Loader Version: {} (Geometry Dash v{})\n"
                "- Loader Commit: {}\n"
                "- Bindings Commit: {}\n"
                "- Installed Mods: {} (Loaded: {}/{})\n"
                "- Problems: {}{}",
                wd, getLoaderVersion(), GEODE_STR(GEODE_GD_VERSION),
                about::getLoaderCommitHash(), about::getBindingsCommitHash(),
                getModCount(), getLoadedModCount(), getEnabledModCount(), problems.size(),
                problems.empty() ? "" : fmt::format("\n{}", readProblems(problems))
        );

        return message;
    }

    std::string getLoaderVersion() {
        return ::geode::Loader::get()->getVersion().toString();
    }

    uint32_t getModCount() {
        return ::geode::Loader::get()->getAllMods().size();
    }

    uint32_t getLoadedModCount() {
        auto mods = ::geode::Loader::get()->getAllMods();
        return std::count_if(mods.begin(), mods.end(), [](const ::geode::Mod *mod) {
            return mod->isEnabled();
        });
    }

    uint32_t getEnabledModCount() {
        auto mods = ::geode::Loader::get()->getAllMods();
        return std::count_if(mods.begin(), mods.end(), [](const ::geode::Mod *mod) {
            return mod->shouldLoad();
        });
    }

    ghc::filesystem::path getCrashlogsPath() {
        return ::geode::dirs::getCrashlogsDir();
    }

    const std::vector<ModInfo> &getModList() {
        static std::vector<ModInfo> mods;
        if (!mods.empty()) {
            return mods;
        }

        auto allMods = ::geode::Loader::get()->getAllMods();
        mods.reserve(allMods.size());

        // Sort the mods by id
        std::sort(allMods.begin(), allMods.end(), [](const ::geode::Mod *a, const ::geode::Mod *b) {
            return a->getMetadata().getID() < b->getMetadata().getID();
        });

        for (auto &mod: allMods) {
            auto metadata = mod->getMetadata();
            auto developers = metadata.getDevelopers();
            std::string developer;
            // Combine all developers into one string
            for (auto it = developers.begin(); it != developers.end(); it++) {
                developer += *it;
                if (std::next(it) != developers.end()) {
                    developer += ", ";
                }
            }

            ModStatus status = ModStatus::Disabled;
            if (mod->isCurrentlyLoading()) {
                status = ModStatus::IsCurrentlyLoading;
            } else if (mod->isEnabled()) {
                status = ModStatus::Enabled;
            } else if (mod->hasProblems()) {
                status = ModStatus::HasProblems;
            } else if (mod->shouldLoad()) {
                status = ModStatus::ShouldLoad;
            }

            mods.push_back({
                metadata.getName(),
                metadata.getID(),
                metadata.getVersion().toString(),
                developer,
                status,
                mod
            });
        }

        return mods;
    }

    const std::string &getModListMessage() {
        static std::string message;
        if (!message.empty()) {
            return message;
        }

        auto mods = getModList();
        std::string result;

        for (auto &mod: mods) {
            char status;
            switch (mod.status) {
                case ModStatus::Disabled:
                    status = ' ';
                    break;
                case ModStatus::IsCurrentlyLoading:
                    status = 'o';
                    break;
                case ModStatus::Enabled:
                    status = 'x';
                    break;
                case ModStatus::HasProblems:
                    status = '!';
                    break;
                case ModStatus::ShouldLoad:
                    status = '~';
                    break;
            }
            result += fmt::format("{} | [{}] {}\n", status, mod.version, mod.id);
        }

        if (!result.empty()) {
            result.pop_back();
        }

        message = result;
        return message;
    }

    const std::unordered_map<uintptr_t, std::string>& getFunctionAddresses() {
        static std::unordered_map<uintptr_t, std::string> functions;
        if (!functions.empty()) {
            return functions;
        }

        auto codegenPath = ::geode::Mod::get()->getResourcesDir() / "CodegenData.txt";
        if (!ghc::filesystem::exists(codegenPath)) {
            return functions;
        }

        std::ifstream file(codegenPath);
        if (!file.is_open()) {
            return functions;
        }

        std::string line;
        while (std::getline(file, line)) {
            auto pos = line.find(" - ");
            if (pos == std::string::npos) {
                continue;
            }
            auto address = std::stoull(line.substr(pos + 3), nullptr, 16);
            functions[address] = line.substr(0, pos);
        }

        return functions;
    }
}