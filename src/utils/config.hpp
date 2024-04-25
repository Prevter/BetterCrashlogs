#pragma once

namespace config {

    struct Config {
        int window_x;
        int window_y;
        int window_w;
        int window_h;
        bool window_maximized;
        float ui_scale;
    };

    void load();
    void save();

    Config& get();

}