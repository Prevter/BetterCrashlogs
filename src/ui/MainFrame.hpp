#pragma once
#include "common.hpp"

namespace breakdown { class CrashHandler; }

class MainFrame final : public wxFrame {
public:
    MainFrame(breakdown::CrashHandler* crash_handler);

private:
    wxMenuBar* setupMenu();

    // callbacks
    void OnCopyCrashlog(wxCommandEvent& event);
    void OnOpenCrashlog(wxCommandEvent& event);
    void OnRestartGame(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);

    breakdown::CrashHandler* m_crashHandler;
};
