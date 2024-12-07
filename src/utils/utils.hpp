#pragma once
#include <random>

#include "witty.hpp"

namespace breakdown::utils {

    std::mt19937& randomEngine();

    template <typename T>
    concept Number = std::integral<T> || std::floating_point<T>;

    template <Number T>
    T random(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(randomEngine());
    }

    inline std::string_view getWittyComment() {
        return WITTY_COMMENTS[random<size_t>(0, WITTY_COMMENTS.size() - 1)];
    }

    std::string formatTime(std::chrono::time_point<std::chrono::system_clock> time, bool fileSafe = false);

}