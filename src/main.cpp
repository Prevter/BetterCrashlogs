#include "ui/App.hpp"
wxIMPLEMENT_APP_NO_MAIN(App);

#include "platform/platform.hpp"

$on_mod(Loaded) {
    breakdown::platform::registerCrashHandler([](auto const& crashHandler) {
        breakdown::g_crashHandler = const_cast<breakdown::CrashHandler*>(&crashHandler);
        geode::log::error("Geometry Dash crashed! Handling crash...");

        // wxEntry locks the thread, so g_crashHandler will be valid until the end of the program
        GEODE_MACOS(int argc = 0;)
        wxEntry(
            // macOS doesn't have default arguments for wxEntry somehow
            GEODE_MACOS(argc, (char**)nullptr)
        );

        breakdown::g_crashHandler = nullptr;
    });
}


