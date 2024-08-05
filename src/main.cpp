#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
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
#include "utils/memory.hpp"
#include "utils/hwinfo.hpp"

#define LOG_WRAP(message, ...) geode::log::info("Getting " message); __VA_ARGS__

std::string getCrashReport(analyzer::Analyzer& analyzer) {
    auto currentDateTime = utils::getCurrentDateTime();
    auto randomQuote = ui::pickRandomQuote();
    LOG_WRAP("Loader Metadata", auto loaderMetadata = utils::geode::getLoaderMetadataMessage());
    LOG_WRAP("Exception Info", auto exceptionInfo = analyzer.getExceptionMessage());
    LOG_WRAP("Stack Trace", auto stackTrace = analyzer.getStackTraceMessage());
    LOG_WRAP("Register States", auto registerStates = analyzer.getRegisterStateMessage());
    LOG_WRAP("Installed Mods", auto installedMods = utils::geode::getModListMessage());
    LOG_WRAP("Stack Allocations", auto stackAllocations = analyzer.getStackAllocationsMessage());
    LOG_WRAP("Hardware Information", auto hardwareInfo = hwinfo::getMessage());

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
            "{}\n\n"
            "== Hardware Information ==\n"
            "{}",
            currentDateTime, randomQuote,
            loaderMetadata, exceptionInfo,
            stackTrace, registerStates,
            installedMods, stackAllocations,
            hardwareInfo
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

geode::EventListener<::geode::utils::web::WebTask> s_listener;

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
        geode::log::info("Saving crash information...");
        std::filesystem::create_directories(crashReportPath.parent_path());
        std::ofstream crashReportFile(crashReportPath);
        crashReportFile << crashReport;
        crashReportFile.close();
        saved = true;
        geode::log::info("Crash information saved to: {}", crashReportPath.string());

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
        auto& cfg = config::get();

        // Top-bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("Copy Crashlog")) {
                ImGui::SetClipboardText(crashReport.c_str());
                ui::showToast("Copied crash information to clipboard.");
            }

            if (ImGui::MenuItem("Open Crashlogs Folder")) {
                ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select," + crashReportPath.wstring()).c_str(),
                              nullptr, SW_SHOWNORMAL);
                ui::showToast(fmt::format("Opened {} in explorer", crashReportPath.string()));
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
                ui::showToast("Analyzer reloaded.");
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

#ifndef _WIN64
            if (!win32::four_gb::isPatched()) {
                if (ImGui::MenuItem("Apply 4GB Patch")) {
                    win32::four_gb::patch();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Patch the game to use 4GB of memory.");
                }
            }
#endif

            // Settings menu
            if (ImGui::BeginMenu("Settings")) {
                // Font scale
                if (ImGui::DragFloat("Font Scale", &cfg.ui_scale, 0.01f, 0.5f, 2.0f)) {
                    config::save();
                    ui::resize();
                }

                if (ImGui::MenuItem("Reset Layout")) {
                    resetLayout(&window);
                    ui::showToast("Reset layout to default.");
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Reset all widgets to their default positions.");
                }

#define TOGGLE(name, var) if (ImGui::MenuItem("Show " #name, nullptr, &cfg.show_##var)) { config::save(); }
                TOGGLE(Information, info);
                TOGGLE(Metadata, meta);
                TOGGLE(Registers, registers);
                TOGGLE(Mods, mods);
                TOGGLE(Stack, stack);
                TOGGLE(StackTrace, stacktrace);
                TOGGLE(Disassembly, disassembly);
#undef TOGGLE

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_C)) {
            ImGui::SetClipboardText(crashReport.c_str());
            ui::showToast("Copied crash information to clipboard.");
        }

        // Windows
        if (cfg.show_info) ui::informationWindow(analyzer);
        if (cfg.show_meta) ui::metadataWindow();
        if (cfg.show_registers) ui::registersWindow(analyzer);
        if (cfg.show_mods) ui::modsWindow();
        if (cfg.show_stack) ui::stackWindow(analyzer);
        if (cfg.show_stacktrace) ui::stackTraceWindow(analyzer);
        if (cfg.show_disassembly) ui::disassemblyWindow(analyzer);

        ui::renderToasts();

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

LONG WINAPI ContinueHandler(LPEXCEPTION_POINTERS info) {
    if (info->ExceptionRecord->ExceptionCode == EH_EXCEPTION_NUMBER) {
        HandleCrash(info);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI ExceptionHandler(LPEXCEPTION_POINTERS info) {
    switch (info->ExceptionRecord->ExceptionCode) {
        case DBG_EXCEPTION_HANDLED:
        case DBG_CONTINUE:
        case DBG_REPLY_LATER:
        case DBG_TERMINATE_THREAD:
        case DBG_TERMINATE_PROCESS:
        case DBG_RIPEXCEPTION:
        case DBG_CONTROL_BREAK:
        case DBG_COMMAND_EXCEPTION:
        case DBG_PRINTEXCEPTION_C:
        case DBG_PRINTEXCEPTION_WIDE_C:
        case DBG_CONTROL_C:
        case STATUS_CONTROL_C_EXIT:
        case EXCEPTION_SET_THREAD_NAME:
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_S_CALL_FAILED:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_PROTOCOL_ERROR:
        case (DWORD)RPC_E_WRONG_THREAD:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_INVALID_STRING_BINDING:
        case RPC_S_WRONG_KIND_OF_BINDING:
        case RPC_S_INVALID_BINDING:
            return EXCEPTION_CONTINUE_SEARCH;
        default:
            return HandleCrash(info);
    }
}

static void updateFile(const std::string& filename) {
    auto req = geode::utils::web::WebRequest();
    req.get(utils::geode::formatFileURL(filename)).listen(
        [filename](auto res) {
            if (!res) return;
            auto data = res->string().unwrapOr("");
            if (data.empty()) return;

            auto path = utils::geode::getConfigPath() / filename;
            std::ofstream file(path);
            file << data;
            file.close();

            geode::log::info("Successfully downloaded {} to {}", filename, path.string());
        },
        [](auto){}, []{}
    );
}

$execute {
    // Copy "imgui.ini" from the resources directory to the config directory if it doesn't exist
    auto resourcesDir = utils::geode::getResourcesPath();
    auto configDir = utils::geode::getConfigPath();
    auto iniPath = (configDir / "imgui.ini");
    if (!std::filesystem::exists(iniPath)) {
        std::filesystem::copy(resourcesDir / "imgui.ini", iniPath);
    }

    // Check if have libcocos2d.txt file
    auto cocosPath = configDir / utils::geode::getCocosFile();
    if (!std::filesystem::exists(cocosPath)) {
        geode::log::info("Fetching libcocos2d symbols...");
        updateFile(utils::geode::getCocosFile());
    }

    // Fetch codegen file once every 4 hours
    auto& config = config::get();
    auto lastUpdate = config.last_bindings_update;
    auto codegenPath = configDir / utils::geode::getBindingsFile();
    if (lastUpdate + 14400 < time(nullptr) || !std::filesystem::exists(codegenPath)) {
        config.last_bindings_update = time(nullptr);
        config::save();
        geode::log::info("Fetching codegen symbols...");
        updateFile(utils::geode::getBindingsFile());
    }

    geode::log::info("Setting up crash handler...");
    SetUnhandledExceptionFilter(ExceptionHandler);

    if (utils::geode::intrusiveEnabled()) {
        geode::queueInMainThread([] {
            geode::log::info("Intrusive mode enabled, setting up continue handler...");
            AddVectoredContinueHandler(0, ContinueHandler);
            AddVectoredExceptionHandler(0, ExceptionHandler);
        });
    }
}

