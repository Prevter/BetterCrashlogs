#include "utils.hpp"

namespace breakdown::utils {

    std::mt19937& randomEngine() {
        thread_local std::mt19937 gen(std::random_device{}());
        return gen;
    }

    std::string formatTime(std::chrono::time_point<std::chrono::system_clock> time, bool fileSafe) {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto tm = fmt::localtime(time_t);
        return fileSafe ? fmt::format("{:%F_%H-%M-%S}", tm) : fmt::format("{:%FT%T%z}", tm);
    }
}
