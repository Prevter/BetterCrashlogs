#include <Geode/Geode.hpp>
#include <imgui.h>

#include "analyzer/analyzer.hpp"
#include "gui/window.hpp"
#include "gui/ui.hpp"
#include "utils/geode-util.hpp"
#include "utils/utils.hpp"
#include "analyzer/exception-codes.hpp"

std::string getCrashReport() {
    return fmt::format(
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
            analyzer::getStackTraceMessage(),
            analyzer::getRegisterStateMessage(),
            utils::geode::getModListMessage(),
            analyzer::getStackAllocationsMessage()
    );
}


LONG WINAPI HandleCrash(LPEXCEPTION_POINTERS ExceptionInfo) {
    // Play a Windows error sound
    MessageBeep(MB_ICONERROR);

    // Prepare the crash information
    analyzer::analyze(ExceptionInfo);

    // Save the crash report
    static bool saved = false;
    static ghc::filesystem::path crashReportPath;
    if (!saved) {
        // Create the crash report
        auto crashReport = getCrashReport();
        geode::log::error("Saving crash information...");
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
                ImGui::SetClipboardText(getCrashReport().c_str());
            }

            if (ImGui::MenuItem("Open Crashlogs Folder")) {
                ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select," + crashReportPath.wstring()).c_str(),
                              nullptr, SW_SHOWNORMAL);
            }

            if (ImGui::MenuItem("Restart Game")) {
                geode::utils::game::restart();
                window.close();
            }

            if (ImGui::MenuItem("Reload Analyzer")) {
                analyzer::reload();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Refresh the crash information (reload all debug symbols).");
            }

            if (ImGui::MenuItem("Step Over")) {
                geode::log::info("Attempting to continue the execution...");
                result = EXCEPTION_CONTINUE_EXECUTION;

                // Skip the instruction
                ExceptionInfo->ContextRecord->Eip += 1;

                window.close();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Attempt to continue the execution of the game.\n"
                                  "In most cases, this will just crash the game again.");
            }

            // This is kinda broken right now but eh?
            auto &stackTrace = analyzer::getStackTrace();
            if (stackTrace.size() > 1) {
                if (ImGui::MenuItem("Step Out")) {
                    result = EXCEPTION_CONTINUE_SEARCH;

                    // Get the address of the latest function call
                    auto returnAddress = stackTrace[1].address;
                    geode::log::info("Attempting to step out of the function at 0x{:X}...", returnAddress);

                    // Restore the context to the caller
                    ExceptionInfo->ContextRecord->Eip = returnAddress;
                    ExceptionInfo->ContextRecord->Esp = stackTrace[1].framePointer;

                    window.close();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Attempt to step out of the function that caused the exception."
                                      "In most cases, this will just crash the game again.");
                }
            }

        }

        // Windows
        ui::informationWindow();
        ui::metadataWindow();
        ui::registersWindow();
        ui::modsWindow();
        ui::stackWindow();
        ui::stackTraceWindow();

    });

    window.init();
    window.run();

    analyzer::cleanup();
    return result;
}

$execute {
    // Copy "imgui.ini" from the resources directory to the config directory if it doesn't exist
    auto resourcesDir = geode::Mod::get()->getResourcesDir();
    auto configDir = geode::Mod::get()->getConfigDir();
    auto iniPath = (configDir / "imgui.ini");
    if (!ghc::filesystem::exists(iniPath)) {
        ghc::filesystem::copy_file(resourcesDir / "imgui.ini", iniPath);
    }

    AddVectoredExceptionHandler(0, [](PEXCEPTION_POINTERS ExceptionInfo) -> LONG {
        auto exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
        if (exceptionCode == EXCEPTION_BREAKPOINT) {
            return HandleCrash(ExceptionInfo);
        } else if (exceptionCode == 0x406D1388) { // Set thread name
            return EXCEPTION_CONTINUE_SEARCH;
        } else {
            geode::log::debug("Got an exception: {} (0x{:X})",
                              analyzer::exceptions::getName(ExceptionInfo->ExceptionRecord->ExceptionCode),
                              ExceptionInfo->ExceptionRecord->ExceptionCode);
            SetUnhandledExceptionFilter(HandleCrash);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    });
    SetUnhandledExceptionFilter(HandleCrash);
}

