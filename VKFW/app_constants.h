/**
 * @file   constants.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Contains global constant definitions.
 */

#pragma once

namespace vkuapp {
    /** The application name. */
    static const char* applicationName = "VKFWApp";
    /** The application version. */
    static const uint32_t applicationVersionMajor = 0;
    static const uint32_t applicationVersionMinor = 1;
    static const uint32_t applicationVersionPatch = 0;
    static const uint32_t applicationVersion = (((applicationVersionMajor) << 22) | ((applicationVersionMinor) << 12) | (applicationVersionPatch));

    /** The configuration file name. */
    static const char* configFileName = "VKFWConfig.xml";
    /** The log file name. */
    static const char* logFileName = "application";
    /** Use a timestamp for the log files. */
    static bool LOG_USE_TIMESTAMPS = false;
}
