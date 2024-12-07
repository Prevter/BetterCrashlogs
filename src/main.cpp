#include "ui/App.hpp"
wxIMPLEMENT_APP_NO_MAIN(App);

#include "platform/platform.hpp"

$on_mod(Loaded) {
    breakdown::platform::registerCrashHandler([](auto const& crashHandler) {
        breakdown::g_crashHandler = const_cast<breakdown::CrashHandler*>(&crashHandler);
        geode::log::error("Geometry Dash crashed! Handling crash...");
        wxEntry(); // wxEntry locks the thread, so g_crashHandler will be valid until the end of the program
        breakdown::g_crashHandler = nullptr;
    });
}


