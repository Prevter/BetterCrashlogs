#include "ui.hpp"

#include "../utils/utils.hpp"
#include "../utils/geode-util.hpp"
#include "../analyzer/disassembler.hpp"
#include "../utils/config.hpp"

#include <imgui.h>

namespace ui {

    static ImFont *mainFont = nullptr;
    static ImFont *titleFont = nullptr;

    static std::unordered_map<const char *, ImU32> colorMap = {
            {"primary",  IM_COL32(183, 148, 246, 255)}, /* purple */
            {"string",   IM_COL32(61, 146, 61, 255)}, /* green */
            {"pointer",  IM_COL32(248, 132, 120, 255)}, /* red */
            {"white",    IM_COL32(255, 255, 255, 255)}, /* white */
            {"address",  IM_COL32(122, 160, 141, 255)}, /* cyan */
            {"function", IM_COL32(245, 86, 119, 255)} /* pink */
    };

    static int quoteIndex = -1; // -1 means uninitialized (we can reset to -1 to pick a new quote)

    const char *pickRandomQuote() {
        if (quoteIndex == -1) // First time
            quoteIndex = utils::randInt(0, RANDOM_MESSAGES_COUNT);
        return RANDOM_MESSAGES[quoteIndex];
    }

    void newQuote() {
        quoteIndex = -1;
    }

    void applyStyles() {
        auto &style = ImGui::GetStyle();
        style.ChildRounding = 0;
        style.GrabRounding = 0;
        style.FrameRounding = 0;
        style.PopupRounding = 0;
        style.ScrollbarRounding = 0;
        style.TabRounding = 0;
        style.WindowRounding = 0;
        style.FramePadding = {4, 4};

        style.WindowTitleAlign = {0.5, 0.5};

        ImVec4 *colors = ImGui::GetStyle().Colors;
        // Updated to use IM_COL32 for more precise colors and to add table colors (1.80 feature)
        colors[ImGuiCol_Text] = ImColor{IM_COL32(0xeb, 0xdb, 0xb2, 0xFF)};
        colors[ImGuiCol_TextDisabled] = ImColor{IM_COL32(0x92, 0x83, 0x74, 0xFF)};
        colors[ImGuiCol_WindowBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xF0)};
        colors[ImGuiCol_ChildBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_PopupBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xF0)};
        colors[ImGuiCol_Border] = ImColor{IM_COL32(0x2d, 0x30, 0x31, 0xFF)};
        colors[ImGuiCol_BorderShadow] = ImColor{0};
        colors[ImGuiCol_FrameBg] = ImColor{IM_COL32(0x3c, 0x38, 0x36, 0x90)};
        colors[ImGuiCol_FrameBgHovered] = ImColor{IM_COL32(0x50, 0x49, 0x45, 0xFF)};
        colors[ImGuiCol_FrameBgActive] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0xA8)};
        colors[ImGuiCol_TitleBg] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0xFF)};
        colors[ImGuiCol_TitleBgActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_TitleBgCollapsed] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x9C)};
        colors[ImGuiCol_MenuBarBg] = ImColor{IM_COL32(0x28, 0x28, 0x28, 0xF0)};
        colors[ImGuiCol_ScrollbarBg] = ImColor{IM_COL32(0x00, 0x00, 0x00, 0x28)};
        colors[ImGuiCol_ScrollbarGrab] = ImColor{IM_COL32(0x3c, 0x38, 0x36, 0xFF)};
        colors[ImGuiCol_ScrollbarGrabHovered] = ImColor{IM_COL32(0x50, 0x49, 0x45, 0xFF)};
        colors[ImGuiCol_ScrollbarGrabActive] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0xFF)};
        colors[ImGuiCol_CheckMark] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x9E)};
        colors[ImGuiCol_SliderGrab] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x70)};
        colors[ImGuiCol_SliderGrabActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_Button] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x66)};
        colors[ImGuiCol_ButtonHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0x9E)};
        colors[ImGuiCol_ButtonActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_Header] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0.4F)};
        colors[ImGuiCol_HeaderHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xCC)};
        colors[ImGuiCol_HeaderActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_Separator] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0.50f)};
        colors[ImGuiCol_SeparatorHovered] = ImColor{IM_COL32(0x50, 0x49, 0x45, 0.78f)};
        colors[ImGuiCol_SeparatorActive] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0xFF)};
        colors[ImGuiCol_ResizeGrip] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x40)};
        colors[ImGuiCol_ResizeGripHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xAA)};
        colors[ImGuiCol_ResizeGripActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xF2)};
        colors[ImGuiCol_Tab] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0xD8)};
        colors[ImGuiCol_TabHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xCC)};
        colors[ImGuiCol_TabActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_TabUnfocused] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0.97f)};
        colors[ImGuiCol_TabUnfocusedActive] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_PlotLines] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0xFF)};
        colors[ImGuiCol_PlotLinesHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_PlotHistogram] = ImColor{IM_COL32(0x98, 0x97, 0x1a, 0xFF)};
        colors[ImGuiCol_PlotHistogramHovered] = ImColor{IM_COL32(0xb8, 0xbb, 0x26, 0xFF)};
        colors[ImGuiCol_TextSelectedBg] = ImColor{IM_COL32(0x45, 0x85, 0x88, 0x59)};
        colors[ImGuiCol_DragDropTarget] = ImColor{IM_COL32(0x98, 0x97, 0x1a, 0.90f)};
        colors[ImGuiCol_TableHeaderBg] = ImColor{IM_COL32(0x38, 0x3c, 0x36, 0xFF)};
        colors[ImGuiCol_TableBorderStrong] = ImColor{IM_COL32(0x28, 0x28, 0x28, 0xFF)};
        colors[ImGuiCol_TableBorderLight] = ImColor{IM_COL32(0x38, 0x3c, 0x36, 0xFF)};
        colors[ImGuiCol_TableRowBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_TableRowBgAlt] = ImColor{IM_COL32(0x28, 0x28, 0x28, 0xFF)};
        colors[ImGuiCol_TextSelectedBg] = ImColor{IM_COL32(0x45, 0x85, 0x88, 0xF0)};
        colors[ImGuiCol_NavHighlight] = ImColor{IM_COL32(0x83, 0xa5, 0x98, 0xFF)};
        colors[ImGuiCol_NavWindowingHighlight] = ImColor{IM_COL32(0xfb, 0xf1, 0xc7, 0xB2)};
        colors[ImGuiCol_NavWindowingDimBg] = ImColor{IM_COL32(0x7c, 0x6f, 0x64, 0x33)};
        colors[ImGuiCol_ModalWindowDimBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0x59)};
    }

    void resize() {
        auto &cfg = config::get();

        auto &io = ImGui::GetIO();
        auto &style = ImGui::GetStyle();

        //applyStyles();
        // style.ScaleAllSizes(cfg.ui_scale);

        io.FontGlobalScale = cfg.ui_scale;
    }

    void informationWindow(analyzer::Analyzer& analyzer) {
        if (ImGui::Begin("Exception Information")) {
            ImGui::PushFont(titleFont);
            ImGui::TextWrapped("%s", pickRandomQuote());
            ImGui::PopFont();
            ImGui::TextWrapped("%s", analyzer.getExceptionMessage().c_str());
        }
        ImGui::End();
    }

    void metadataWindow() {
        if (ImGui::Begin("Geode Information")) {
            ImGui::TextWrapped("%s", utils::geode::getLoaderMetadataMessage().c_str());
        }
        ImGui::End();
    }

    void registersWindow(analyzer::Analyzer& analyzer) {
        if (ImGui::Begin("Register States")) {

            auto registers = analyzer.getRegisterStates();

            // Create a table with the register states
            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
                                    ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_ScrollX;
            ImVec2 size = ImVec2(0, 11 * ImGui::GetTextLineHeightWithSpacing());
            if (ImGui::BeginTable("registers", 3, flags, size)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                ImGui::TableSetupColumn("Description");
                ImGui::TableHeadersRow();

                for (const auto &reg: registers) {
                    ImGui::TableNextColumn();

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                    ImGui::Text("%s", reg.name.c_str());
                    ImGui::PopStyleColor();

                    ImGui::TableNextColumn();

                    if (reg.type != analyzer::ValueType::Unknown) {
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["pointer"]);
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                    }
                    ImGui::Text("%08X", reg.value);
                    ImGui::PopStyleColor();

                    ImGui::TableNextColumn();

                    switch (reg.type) {
                        case analyzer::ValueType::Pointer:
                            ImGui::PushStyleColor(ImGuiCol_Text, colorMap["address"]);
                            break;
                        case analyzer::ValueType::Function:
                            ImGui::PushStyleColor(ImGuiCol_Text, colorMap["function"]);
                            break;
                        case analyzer::ValueType::String:
                            ImGui::PushStyleColor(ImGuiCol_Text, colorMap["string"]);
                            break;
                        default:
                            ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                            break;
                    }
                    ImGui::Text("%s", reg.description.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::EndTable();
            }

            // Flags table
            if (ImGui::BeginTable("flags", 6,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
                                  ImGuiTableFlags_NoHostExtendX)) {
                ImGui::TableSetupColumn("Flag", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Flag", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Flag", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                auto context = analyzer.getCpuFlags();
                int i = 0;
                for (const auto &[flag, value]: context) {
                    ImGui::TableNextColumn();

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                    ImGui::Text("%s", flag.c_str());
                    ImGui::PopStyleColor();

                    ImGui::TableNextColumn();

                    ImGui::PushStyleColor(ImGuiCol_Text, value ? colorMap["string"] : colorMap["white"]);
                    ImGui::Text("%s", value ? "1" : "0");
                    ImGui::PopStyleColor();

                    if (++i % 3 == 0 && i < context.size()) {
                        ImGui::TableNextRow();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void modsWindow() {
        if (ImGui::Begin("Installed Mods")) {

            auto mods = utils::geode::getModList();

            // Create a table with the installed mods
            ImGui::BeginTable("mods", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                         ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX |
                                         ImGuiTableFlags_NoHostExtendY);
            ImGui::TableSetupColumn("Version");
            ImGui::TableSetupColumn("Mod ID");
            ImGui::TableHeadersRow();

            int i = 0;
            for (const auto &mod: mods) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                std::string statusStr;
                std::string tooltipStr;

                ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                ImGui::Text("%s", mod.version.c_str());
                ImGui::PopStyleColor();


#define STATUS_CASE(status, color, text, tooltip)              \
    case utils::geode::ModStatus::status:                      \
        ImGui::PushStyleColor(ImGuiCol_Text, colorMap[color]); \
        statusStr = text;                                      \
        tooltipStr = tooltip;                                  \
        break;

                switch (mod.status) {
                    STATUS_CASE(Disabled, "address", " ", "The mod is disabled.")
                    STATUS_CASE(IsCurrentlyLoading, "function", "o",
                                "The mod is currently loading.\nThis means the crash may be related to this mod.")
                    STATUS_CASE(Enabled, "string", "x", "The mod is enabled.")
                    STATUS_CASE(HasProblems, "primary", "!", "The mod has problems.")
                    STATUS_CASE(ShouldLoad, "white", "~", "The mod is expected to be loaded.")
                }

#undef STATUS_CASE

                ImGui::TableNextColumn();
                ImGui::Text("%s", mod.id.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", tooltipStr.c_str());
                }
                ImGui::PopStyleColor();
            }

            ImGui::EndTable();
        }
        ImGui::End();
    }

    void stackWindow(analyzer::Analyzer& analyzer) {
        if (ImGui::Begin("Stack Allocations")) {

            auto stackAlloc = analyzer.getStackData();

            // Create a table with the stack trace
            ImGui::BeginTable("stack", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoHostExtendY);
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Description");
            ImGui::TableHeadersRow();

            int i = 0;
            for (const auto &line: stackAlloc) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                ImGui::Text("%08X", line.address);
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();

                if (line.type != analyzer::ValueType::Unknown) {
                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["pointer"]);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                }
                ImGui::Text("%08X", line.value);
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();

                switch (line.type) {
                    case analyzer::ValueType::Pointer:
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["address"]);
                        break;
                    case analyzer::ValueType::Function:
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["function"]);
                        break;
                    case analyzer::ValueType::String:
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["string"]);
                        break;
                    default:
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                        break;
                }
                ImGui::Text("%s", line.description.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndTable();
        }
        ImGui::End();
    }

    uint32_t disassembledStackTraceIndex = 0;

    void stackTraceWindow(analyzer::Analyzer& analyzer) {
        if (ImGui::Begin("Stack Trace", nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {

            auto stackTrace = analyzer.getStackTrace();

#define COPY_POPUP(value, id)                        \
        if (ImGui::BeginPopupContextItem(id)) {      \
            ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);\
            if (ImGui::MenuItem("Copy")) {           \
                ImGui::SetClipboardText(value);      \
            }                                        \
            ImGui::PopStyleColor();                  \
            ImGui::EndPopup();                       \
        }

            // Create a table with the stack trace
            for (const auto &line: stackTrace) {
                ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                auto functionStr = line.function.toString();
                if (line.function.module.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                    ImGui::Text("- %s", functionStr.c_str());
                    COPY_POPUP(functionStr.c_str(), fmt::format("address_{:X}", line.address).c_str());
                    ImGui::PopStyleColor();
                } else if (ImGui::TreeNode(functionStr.c_str())) {
                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                    ImGui::Text("Address");
                    ImGui::PopStyleColor();

                    ImGui::SameLine();

                    auto addressStr = fmt::format("{}+0x{:X} (0x{:X})", line.module.name, line.moduleOffset,
                                                  line.address);
                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                    ImGui::Text("%s", addressStr.c_str());
                    ImGui::PopStyleColor();

                    COPY_POPUP(addressStr.c_str(), addressStr.c_str());

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                    ImGui::Text("Function");
                    ImGui::PopStyleColor();

                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["function"]);
                    ImGui::Text("%s", functionStr.c_str());
                    COPY_POPUP(functionStr.c_str(), functionStr.c_str());
                    ImGui::PopStyleColor();

                    if (!line.function.file.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                        ImGui::Text("File");
                        ImGui::PopStyleColor();

                        ImGui::SameLine();

                        auto lineStr = fmt::format("{}:{}", line.function.file, line.function.line);
                        ImGui::PushStyleColor(ImGuiCol_Text, colorMap["string"]);
                        ImGui::Text("%s", lineStr.c_str());
                        ImGui::PopStyleColor();

                        COPY_POPUP(lineStr.c_str(), lineStr.c_str());
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                    ImGui::Text("Frame Pointer");
                    ImGui::PopStyleColor();

                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["address"]);
                    ImGui::Text("0x%08X", line.framePointer);
                    ImGui::PopStyleColor();

                    COPY_POPUP(fmt::format("0x{:X}", line.framePointer).c_str(),
                               fmt::format("frame_{:X}", line.framePointer).c_str());

                    ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                    if (ImGui::Button("Disassemble")) {
                        auto index = std::distance(stackTrace.begin(), std::find_if(stackTrace.begin(), stackTrace.end(), [&line](const auto &x) {
                            return x.address == line.address;
                        }));
                        disassembledStackTraceIndex = index;
                    }
                    ImGui::PopStyleColor();

                    ImGui::TreePop();
                }
                ImGui::PopStyleColor();
            }

        }
        ImGui::End();
    }

    void disassemblyWindow(analyzer::Analyzer& analyzer) {
        if (ImGui::Begin("Disassembly", nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
            auto stackTrace = analyzer.getStackTrace();
            if (stackTrace.empty()) {
                ImGui::Text("No stack trace available.");
                ImGui::End();
                return;
            }

            auto stack = stackTrace[disassembledStackTraceIndex];
            if (stack.function.address == 0) {
                ImGui::Text("Cannot disassemble selected function.\nChoose another stack frame.");
                ImGui::End();
                return;
            }

            auto from = stack.address - stack.function.offset;
            auto assembly = disasm::disassemble(from, stack.address + 0x20);

            ImGui::Text("Function: %s", stack.function.toString().c_str());

            ImGui::BeginTable("disassembly", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX |
                                                ImGuiTableFlags_NoHostExtendY);
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Bytes");
            ImGui::TableSetupColumn("Instruction");
            ImGui::TableHeadersRow();

            for (int i = 0; i < assembly.size(); i++) {
                auto ins = assembly[i];
                ImGui::TableNextRow();

                // Color the row red if the address is the next instruction
                if (i + 1 < assembly.size() && assembly[i + 1].address == stack.address) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 0, 0, 100));
                } else {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, i % 2 == 0 ? IM_COL32(255, 255, 255, 8) : IM_COL32(0, 0, 0, 0));
                }

                ImGui::TableNextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, colorMap["address"]);
                ImGui::Text("%08X", ins.address);
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, colorMap["primary"]);
                ImGui::Text("%s", ins.bytes.c_str());
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, colorMap["white"]);
                ImGui::Text("%s", ins.text.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndTable();

        }
        ImGui::End();
    }

}