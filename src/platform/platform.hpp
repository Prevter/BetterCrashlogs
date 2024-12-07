#pragma once

#ifdef _WIN32
#include "windows/Windows.hpp"
#elif __APPLE__
#include "macos/MacOS.hpp"
#endif

#include "../crashhandler/crashhandler.hpp"

namespace breakdown::platform {

    // Execute a function when the application crashes
    void registerCrashHandler(std::function<void(CrashHandler const&)> handler);

}