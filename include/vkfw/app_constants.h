/**
 * @file   constants.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains global constant definitions.
 */

#pragma once

#include <cstdint>

namespace vkfw_app {
    /** The application name. */
    constexpr const char* applicationName = "VKFWApp";
    /** The application version. */
    constexpr std::uint32_t applicationVersionMajor = 0;
    constexpr std::uint32_t applicationVersionMinor = 2;
    constexpr std::uint32_t applicationVersionPatch = 0;
    constexpr std::uint32_t applicationVersion = (((applicationVersionMajor) << 22) | ((applicationVersionMinor) << 12) | (applicationVersionPatch));

    /** The configuration file name. */
    constexpr const char* configFileName = "VKFWConfig.xml";
    /** The log file name. */
    constexpr const char* logFileName = "application.log";
    /** Use a timestamp for the log files. */
    constexpr bool LOG_USE_TIMESTAMPS = false;
    /** Log file application tag. */
    constexpr const char* logTag = "vkfw";
}
