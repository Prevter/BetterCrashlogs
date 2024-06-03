#pragma once

#include <cstdint>
#include <string>

namespace utils {

    /// @brief Generate a random integer between min and max.
    int32_t randInt(int32_t min, int32_t max);

    /// @brief Get the current date and time.
    std::string getCurrentDateTime(bool fileSafe = false);

    /// @brief Get the file name from a path.
    std::string getFileName(const std::string& path);
}