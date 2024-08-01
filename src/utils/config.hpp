#pragma once

#include <ctime>

namespace config {

    struct Config {
        int window_x;
        int window_y;
        int window_w;
        int window_h;
        bool window_maximized;
        float ui_scale;
        time_t last_bindings_update;
        bool show_info;
        bool show_meta;
        bool show_registers;
        bool show_mods;
        bool show_stack;
        bool show_stacktrace;
        bool show_disassembly;
    };

    void load();
    void save();

    Config& get();

}