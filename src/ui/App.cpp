#include "App.hpp"
#include "MainFrame.hpp"
#include "../crashhandler/crashhandler.hpp"

bool App::OnInit() {
    auto frame = new MainFrame(breakdown::g_crashHandler);
    frame->Show(true);
    return true;
}