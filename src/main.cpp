#include <Geode/Geode.hpp>
#include <imgui.h>

#include "analyzer/analyzer.hpp"
#include "gui/window.hpp"
#include "gui/ui.hpp"
#include "utils/geode-util.hpp"
#include "utils/utils.hpp"

LONG WINAPI HandleCrash(LPEXCEPTION_POINTERS ExceptionInfo) {
    // Play a Windows error sound
    MessageBeep(MB_ICONERROR);

    // Prepare the crash information
    analyzer::analyze(ExceptionInfo);

    // Create the crash report
    auto crashReport = fmt::format(
            "{}\n{}\n\n"
            "== Geode Information ==\n"
            "{}\n\n"
            "== Exception Information ==\n"
            "{}\n\n"
            "== Stack Trace ==\n"
            "{}\n\n"
            "== Register States ==\n"
            "{}\n\n"
            "== Installed Mods ==\n"
            "{}\n\n"
            "== Stack Allocations ==\n"
            "{}",
            utils::getCurrentDateTime(),
            ui::pickRandomQuote(),
            utils::geode::getLoaderMetadataMessage(),
            analyzer::getExceptionMessage(),
            "Not implemented yet.", // analyzer::getStackTraceMessage(),
            analyzer::getRegisterStateMessage(),
            utils::geode::getModListMessage(),
            analyzer::getStackAllocationsMessage()
    );

    // Save the crash report
    static bool saved = false;
    static ghc::filesystem::path crashReportPath;
    if (!saved) {
        geode::log::error("An exception occurred! Saving crash information...");
        saved = true;
        crashReportPath = utils::geode::getCrashlogsPath() / fmt::format("{}.txt", utils::getCurrentDateTime(true));
        ghc::filesystem::create_directories(crashReportPath.parent_path());
        ghc::filesystem::ofstream crashReportFile(crashReportPath);
        crashReportFile << crashReport;
        crashReportFile.close();
    }

    // Set up the paths
    static auto resourcesDir = geode::Mod::get()->getResourcesDir();
    static auto configDir = geode::Mod::get()->getConfigDir();
    static const auto iniPath = (configDir / "imgui.ini").string();
    static const auto fontPath = (resourcesDir / "FantasqueSansMono.ttf").string();

    LONG result = EXCEPTION_CONTINUE_SEARCH;

    // Create the window
    gui::ImGuiWindow window([]() {
        auto &io = ImGui::GetIO();
        io.IniFilename = iniPath.c_str();
        ui::mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f);
        ui::titleFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 22.0f);
        io.FontDefault = ui::mainFont;
        ui::applyStyles();
    }, [&]() {
        // Top-bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("Copy Crashlog")) {
                ImGui::SetClipboardText(crashReport.c_str());
            }

            if (ImGui::MenuItem("Open Crashlogs Folder")) {
                ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select," + crashReportPath.wstring()).c_str(), nullptr, SW_SHOWNORMAL);
            }

            if (ImGui::MenuItem("Reload Analyzer")) {
                analyzer::reload();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Refresh the crash information (reload all debug symbols).");
            }

            if (ImGui::MenuItem("Ignore Exception")) {
                geode::log::info("Attempting to continue the execution...");
                result = EXCEPTION_CONTINUE_EXECUTION;

                // Skip the instruction that caused the exception
                auto context = ExceptionInfo->ContextRecord;

                // Check if the exception was caused by a breakpoint
                if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) {
                    context->Eip++;
                } else {
                    context->Eip += 2;
                }

                analyzer::cleanup();
                window.close();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Attempt to continue the execution of the game.\n"
                                  "In most cases, this will just crash the game again.");
            }
        }

        // Windows
        ui::informationWindow();
        ui::metadataWindow();
        ui::registersWindow();
        ui::modsWindow();
        ui::stackWindow();

    });

    window.init();
    window.run();

    return result;
}

$execute {
    AddVectoredExceptionHandler(0, [](PEXCEPTION_POINTERS ExceptionInfo) -> LONG {
        SetUnhandledExceptionFilter(HandleCrash);
        return EXCEPTION_CONTINUE_SEARCH;
    });
    SetUnhandledExceptionFilter(HandleCrash);
}

