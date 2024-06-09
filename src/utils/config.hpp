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
    };

    void load();
    void save();

    Config& get();

}