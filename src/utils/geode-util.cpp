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
                "- Mods Installed: {} (Loaded: {})"
                "{}",
                wd, getLoaderVersion(), GEODE_STR(GEODE_GD_VERSION),
                getModCount(), getLoadedModCount(),
                problems.empty() ? "" : fmt::format("\n\n- Problems: {}\n{}", problems.size(), readProblems(problems))
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

    ghc::filesystem::path getCrashlogsPath() {
        return ::geode::dirs::getCrashlogsDir();
    }

}