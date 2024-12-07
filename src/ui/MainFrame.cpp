#include "MainFrame.hpp"

#include "../crashhandler/crashhandler.hpp"

enum {
    ID_CopyCrashlog = wxID_HIGHEST + 1,
    ID_OpenCrashlog,
    ID_RestartGame,
};

MainFrame::MainFrame(breakdown::CrashHandler* crash_handler) : wxFrame(nullptr, wxID_ANY, "Oops! Something went wrong.") {
    m_crashHandler = crash_handler;

    wxFrameBase::SetMenuBar(this->setupMenu());

    // add bottom panel
    auto panel = new wxPanel(this);
    auto sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(sizer);

    // add textbox
    auto text = new wxTextCtrl(panel, wxID_ANY, crash_handler->getString(), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    text->SetFont({10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL});
    sizer->Add(text, 1, wxEXPAND | wxALL, 5);

    // add restart button to bottom panel
    auto restartButton = new wxButton(panel, ID_RestartGame, "Restart Game");
    sizer->Add(restartButton, 0, wxALIGN_RIGHT | wxALL, 5);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnRestartGame, this, ID_RestartGame);

    wxFrameBase::CreateStatusBar();
    wxFrameBase::SetStatusText(crash_handler->getWittyComment().data());
}

wxMenuBar* MainFrame::setupMenu() {
    auto fileMenu = new wxMenu;
    fileMenu->Append(ID_CopyCrashlog, "&Copy Crashlog", "Copy the crashlog to the clipboard");
    fileMenu->Append(ID_OpenCrashlog, "&Open Crashlogs Folder", "Show the crashlogs folder in your file explorer");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT);

    auto menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, "&File");

    this->Bind(wxEVT_MENU, &MainFrame::OnCopyCrashlog, this, ID_CopyCrashlog);
    this->Bind(wxEVT_MENU, &MainFrame::OnOpenCrashlog, this, ID_OpenCrashlog);
    this->Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);

    return menuBar;
}

void MainFrame::OnCopyCrashlog(wxCommandEvent&) {
    geode::utils::clipboard::write(m_crashHandler->getString());
}

void MainFrame::OnOpenCrashlog(wxCommandEvent&) {
    geode::utils::file::openFolder(m_crashHandler->getCrashlogPath());
}

void MainFrame::OnRestartGame(wxCommandEvent&) {
    geode::utils::game::restart();
}

void MainFrame::OnExit(wxCommandEvent&) {
    Close(true);
}

// void MainFrame::OnAbout(wxCommandEvent&) {
//     wxMessageBox("This is a wxWidgets Hello World example",
//                  "About Hello World", wxOK | wxICON_INFORMATION);
// }
