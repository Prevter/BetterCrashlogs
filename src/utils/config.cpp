#include "config.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

#include "geode-util.hpp"
#include "../gui/ui.hpp"

namespace config {

    Config& get() {
        static bool loaded = false;
        static Config instance = {
            50, 50, 1280, 720,
            false, 1.f, 0,
            true, true, true,
            true, true, true, true
        };
        if (!loaded) {
            loaded = true;
            load();
        }
        return instance;
    }

    inline std::filesystem::path getConfigPath() {
        return utils::geode::getConfigPath() / "config.ini";
    }

    void load() {
        auto& config = get();
        std::ifstream file(getConfigPath());
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            auto delimiter = line.find('=');
            if (delimiter == std::string::npos) continue;
            auto key = line.substr(0, delimiter);
            auto value = line.substr(delimiter + 1);

            if (key == "window_x") config.window_x = std::stoi(value);
            else if (key == "window_y") config.window_y = std::stoi(value);
            else if (key == "window_w") config.window_w = std::stoi(value);
            else if (key == "window_h") config.window_h = std::stoi(value);
            else if (key == "window_maximized") config.window_maximized = value == "true";
            else if (key == "ui_scale") config.ui_scale = std::stof(value);
            else if (key == "last_bindings_update") config.last_bindings_update = std::stoll(value);
            else if (key == "show_info") config.show_info = value == "true";
            else if (key == "show_meta") config.show_meta = value == "true";
            else if (key == "show_registers") config.show_registers = value == "true";
            else if (key == "show_mods") config.show_mods = value == "true";
            else if (key == "show_stack") config.show_stack = value == "true";
            else if (key == "show_stacktrace") config.show_stacktrace = value == "true";
            else if (key == "show_disassembly") config.show_disassembly = value == "true";
        }

        file.close();
    }

    void save() {
        auto& config = get();
        std::ofstream file(getConfigPath());

        file << "window_x=" << config.window_x << "\n";
        file << "window_y=" << config.window_y << "\n";
        file << "window_w=" << config.window_w << "\n";
        file << "window_h=" << config.window_h << "\n";
        file << "window_maximized=" << (config.window_maximized ? "true" : "false") << "\n";
        file << "ui_scale=" << config.ui_scale << "\n";
        file << "last_bindings_update=" << config.last_bindings_update << "\n";
        file << "show_info=" << (config.show_info ? "true" : "false") << "\n";
        file << "show_meta=" << (config.show_meta ? "true" : "false") << "\n";
        file << "show_registers=" << (config.show_registers ? "true" : "false") << "\n";
        file << "show_mods=" << (config.show_mods ? "true" : "false") << "\n";
        file << "show_stack=" << (config.show_stack ? "true" : "false") << "\n";
        file << "show_stacktrace=" << (config.show_stacktrace ? "true" : "false") << "\n";
        file << "show_disassembly=" << (config.show_disassembly ? "true" : "false") << "\n";

        file.close();
    }

}