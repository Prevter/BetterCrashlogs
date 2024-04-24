#include "config.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

#include "geode-util.hpp"

namespace config {

    Config& get() {
        static bool loaded = false;
        static Config instance = { 50, 50, 1280, 720, false };
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

        file.close();
    }

}