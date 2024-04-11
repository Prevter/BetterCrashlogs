#include "utils.hpp"
#include <random>

namespace utils {

    std::mt19937& getRng() {
        static thread_local std::mt19937 gen(std::random_device{}());
        return gen;
    }

    int32_t randInt(int32_t min, int32_t max) {
        std::uniform_int_distribution<int32_t> dist(min, max);
        return dist(getRng());
    }

    std::string getCurrentDateTime(bool fileSafe) {
        auto const now = std::time(nullptr);
        struct tm tm{};
        localtime_s(&tm, &now);
        std::ostringstream oss;
        if (fileSafe) {
            oss << std::put_time(&tm, "%F_%H-%M-%S");
        } else {
            oss << std::put_time(&tm, "%FT%T%z"); // ISO 8601
        }
        return oss.str();
    }

    std::string getFileName(const std::string& path) {
        auto pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }

}