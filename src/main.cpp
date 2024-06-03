#include <Geode/Geode.hpp>
#include <imgui.h>

#include "analyzer/analyzer.hpp"
#include "gui/window.hpp"
#include "gui/ui.hpp"
#include "utils/geode-util.hpp"
#include "utils/utils.hpp"
#include "analyzer/exception-codes.hpp"
#include "analyzer/disassembler.hpp"
#include "analyzer/4gb_patch.hpp"
#include "utils/config.hpp"

std::string getCrashReport(analyzer::Analyzer& analyzer) {
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
            analyzer.getExceptionMessage(),
            analyzer.getStackTraceMessage(),
            analyzer.getRegisterStateMessage(),
            utils::geode::getModListMessage(),
            analyzer.getStackAllocationsMessage()
    );
}

inline void setProgramCounter(PCONTEXT context, uintptr_t address) {
#ifdef _WIN64
    context->Rip = address;
#else
    context->Eip = address;
#endif
}

#ifndef _WIN64
__declspec(naked) void stepOutOfFunction() {
    // I don't know what I'm doing, just hoping this works lol
    __asm {
        mov esp, ebp
        pop ebp
        ret
    }
}
#endif

///@brief Dead thread is redirected to this function, which will terminate the thread
__declspec(noreturn) void terminateThreadHandler() { ExitThread(0); }

static bool needLayoutReload = false;
void resetLayout(gui::ImGuiWindow* window) {
    // tell imgui to reload the layout
    needLayoutReload = true;
    window->reload();
}

LONG WINAPI HandleCrash(LPEXCEPTION_POINTERS ExceptionInfo) {
    static bool running = false;
    static bool shouldClose = false;
    static std::mutex mutex;

    // if currently running, close the previous crashlog and wait for it to finish
    if (running) shouldClose = true;

    // Lock the mutex
    std::lock_guard lock(mutex);
    running = true;

    // Play a Windows error sound
    MessageBeep(MB_ICONERROR);

    // Set up the paths
    static auto resourcesDir = utils::geode::getResourcesPath();
    static auto configDir = utils::geode::getConfigPath();
    static auto crashReportDir = utils::geode::getCrashlogsPath();
    static const auto iniPath = (configDir / "imgui.ini").string();
    static const auto fontPath = (resourcesDir / "FantasqueSansMono.ttf").string();

    // Analyze the crash
    ui::newQuote();
    static analyzer::Analyzer analyzer;
    analyzer.analyze(ExceptionInfo);
    auto crashReport = getCrashReport(analyzer);

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

        // Create empty "last-crashed" file to indicate that the game crashed
        std::ofstream lastCrashedFile(crashReportDir / "last-crashed");
        lastCrashedFile.close();
    }

    LONG result = EXCEPTION_CONTINUE_SEARCH;

    // Check if that was a graphics driver crash (because window will draw white screen)
    if (analyzer.isGraphicsDriverCrash()) {
        // Fallback to MessageBox if the window doesn't work
        MessageBoxA(nullptr, crashReport.c_str(), "Something went wrong! ~ BetterCrashlogs fallback mode", MB_ICONERROR | MB_OK);
        return result;
    }

    // Create the window
    gui::ImGuiWindow window([]() {
        auto &io = ImGui::GetIO();

        if (needLayoutReload) {
            io.IniFilename = nullptr; // make it reload the layout

            // replace the ini file with the default one
            std::filesystem::remove(iniPath);
            std::filesystem::copy(resourcesDir / "imgui.ini", iniPath);

            // reload the layout
            ImGui::LoadIniSettingsFromDisk(iniPath.c_str());

            needLayoutReload = false;
        }

        io.IniFilename = iniPath.c_str();
        auto characterRanges = io.Fonts->GetGlyphRangesCyrillic();
        ui::getMainFont() = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f, nullptr, characterRanges);
        ui::getTitleFont() = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 22.0f, nullptr, characterRanges);
        io.FontDefault = ui::getMainFont();
        io.MouseWheelFriction = 5.5f;
        ui::applyStyles();
        ui::resize();
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
                // Terminate the thread, so the game can be restarted
                if (!analyzer.isMainThread()) {
                    result = EXCEPTION_CONTINUE_EXECUTION;
                    setProgramCounter(ExceptionInfo->ContextRecord,
                                      reinterpret_cast<uintptr_t>(&terminateThreadHandler));
                }

                window.close();
                geode::utils::game::restart();
            }

            if (ImGui::MenuItem("Reload Analyzer")) {
                analyzer.reload();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Refresh the crash information (reload all debug symbols).");
            }

            if (ImGui::MenuItem("Step Over")) {
                geode::log::info("Attempting to continue the execution...");
                result = EXCEPTION_CONTINUE_EXECUTION;

                // Get the current instruction and increment the program counter
#ifndef _WIN64
                auto &instruction = disasm::disassemble(ExceptionInfo->ContextRecord->Eip);
                ExceptionInfo->ContextRecord->Eip += instruction.size;
#else
                auto &instruction = disasm::disassemble(ExceptionInfo->ContextRecord->Rip);
                ExceptionInfo->ContextRecord->Rip += instruction.size;
#endif

                window.close();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Attempt to continue the execution of the game.\n"
                                  "In most cases, this will just crash the game again.");
            }

#ifndef _WIN64
            // This is kinda broken right now but eh?
            auto &stackTrace = analyzer.getStackTrace();
            if (stackTrace.size() > 1) {
                if (ImGui::MenuItem("Step Out")) {
                    result = EXCEPTION_CONTINUE_EXECUTION;

                    // Restore the context to the caller
                    ExceptionInfo->ContextRecord->Eip = reinterpret_cast<DWORD>(&stepOutOfFunction);
                    ExceptionInfo->ContextRecord->Esp = stackTrace[1].framePointer;
                    ExceptionInfo->ContextRecord->Ebp = stackTrace[1].framePointer;

                    window.close();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Attempt to step out of the function that caused the exception.\n"
                                      "In most cases, this will just crash the game again.");
                }
            }
#endif

            if (!analyzer.isMainThread()) {
                if (ImGui::MenuItem("Terminate Thread")) {
                    geode::log::info("Terminating the thread...");
                    result = EXCEPTION_CONTINUE_EXECUTION;
                    setProgramCounter(ExceptionInfo->ContextRecord, reinterpret_cast<uintptr_t>(&terminateThreadHandler));
                    window.close();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Terminate the thread that caused the exception.\n(Will close the crash handler, without closing the game)");
                }
            }

            if (!win32::four_gb::isPatched()) {
                if (ImGui::MenuItem("Apply 4GB Patch")) {
                    win32::four_gb::patch();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Patch the game to use 4GB of memory.");
                }
            }

            // Settings menu
            if (ImGui::BeginMenu("Settings")) {
                auto& cfg = config::get();

                // Font scale
                if (ImGui::DragFloat("Font Scale", &cfg.ui_scale, 0.01f, 0.5f, 2.0f)) {
                    config::save();
                    ui::resize();
                }

                if (ImGui::MenuItem("Reset Layout")) {
                    resetLayout(&window);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Reset all widgets to their default positions.");
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // Windows
        ui::informationWindow(analyzer);
        ui::metadataWindow();
        ui::registersWindow(analyzer);
        ui::modsWindow();
        ui::stackWindow(analyzer);
        ui::stackTraceWindow(analyzer);
        ui::disassemblyWindow(analyzer);

        // Close the window if the user requested it
        if (shouldClose) {
            result = EXCEPTION_CONTINUE_EXECUTION;
            setProgramCounter(ExceptionInfo->ContextRecord, reinterpret_cast<uintptr_t>(&terminateThreadHandler));
            window.close();
        }
    });

    window.init();
    window.run();

    analyzer.cleanup();
    shouldClose = false;
    running = false;

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

    geode::log::info("Setting up crash handler...");

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

