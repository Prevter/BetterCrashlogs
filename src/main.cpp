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

__declspec(naked) void stepOutOfFunction() {
    // I don't know what I'm doing, just hoping this works lol
    __asm {
            mov esp, ebp
            pop ebp
            ret
    }
}

LONG WINAPI HandleCrash(LPEXCEPTION_POINTERS ExceptionInfo) {
    // Play a Windows error sound
    MessageBeep(MB_ICONERROR);

    // Set up the paths
    static auto resourcesDir = utils::geode::getResourcesPath();
    static auto configDir = utils::geode::getConfigPath();
    static auto crashReportDir = utils::geode::getCrashlogsPath();
    static const auto iniPath = (configDir / "imgui.ini").string();
    static const auto fontPath = (resourcesDir / "FantasqueSansMono.ttf").string();

    // Analyze the crash
    analyzer::analyze(ExceptionInfo);
    auto crashReport = getCrashReport();

    // Save the crash report
    static bool saved = false;
    static auto crashReportPath = crashReportDir / fmt::format("{}.txt", utils::getCurrentDateTime(true));
    if (!saved) {
        // Create the crash report
        geode::log::error("Saving crash information...");
        std::filesystem::create_directories(crashReportPath.parent_path());
        std::ofstream crashReportFile(crashReportPath);
        crashReportFile << crashReport;
        crashReportFile.close();
        saved = true;
    }

    LONG result = EXCEPTION_CONTINUE_SEARCH;

    // Check if that was a graphics driver crash (because window will draw white screen)
    if (analyzer::isGraphicsDriverCrash()) {
        // Fallback to MessageBox if the window doesn't work
        MessageBoxA(nullptr, crashReport.c_str(), "Something went wrong! ~ BetterCrashlogs fallback mode", MB_ICONERROR | MB_OK);
        return result;
    }

    // Create the window
    gui::ImGuiWindow window([]() {
        auto &io = ImGui::GetIO();
        io.IniFilename = iniPath.c_str();
        auto characterRanges = io.Fonts->GetGlyphRangesCyrillic();
        ui::mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f, nullptr, characterRanges);
        ui::titleFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 22.0f, nullptr, characterRanges);
        io.FontDefault = ui::mainFont;
        io.MouseWheelFriction = 5.5f;
        ui::applyStyles();
    }, [&]() {
        // Top-bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("Copy Crashlog")) {
                ImGui::SetClipboardText(crashReport.c_str());
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

                    // Restore the context to the caller
                    ExceptionInfo->ContextRecord->Eip = reinterpret_cast<DWORD>(&stepOutOfFunction);

                    window.close();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Attempt to step out of the function that caused the exception.\n"
                                      "In most cases, this will just crash the game again.");
                }
            }

            ImGui::EndMainMenuBar();
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
    auto resourcesDir = utils::geode::getResourcesPath();
    auto configDir = utils::geode::getConfigPath();
    auto iniPath = (configDir / "imgui.ini");
    if (!std::filesystem::exists(iniPath)) {
        std::filesystem::copy(resourcesDir / "imgui.ini", iniPath);
    }

    AddVectoredExceptionHandler(0, [](PEXCEPTION_POINTERS ExceptionInfo) -> LONG {
        auto exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
        switch (exceptionCode) {
            case EXCEPTION_BREAKPOINT:
            case EXCEPTION_STACK_OVERFLOW:
                return HandleCrash(ExceptionInfo);
            case EXCEPTION_SET_THREAD_NAME:
                return EXCEPTION_CONTINUE_SEARCH;
            default:
                SetUnhandledExceptionFilter(HandleCrash);
                return EXCEPTION_CONTINUE_SEARCH;
        }
    });
    SetUnhandledExceptionFilter(HandleCrash);
}

