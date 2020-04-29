/**
 * @file   main.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.27
 *
 * @brief  Implements the applications entry point for windows.
 */

#include "main.h"
#include "app_constants.h"
#include "app/FWApplication.h"

#include <core/spdlog/sinks/filesink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>


int main(int /* argc */, const char** /* argv */)
{
    try {
        constexpr std::string_view directory = "";
        constexpr std::string_view name = vkfw_app::logFileName;

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern(fmt::format("[{}] [%^%l%$] %v", vkfw_app::logTag));

        auto devenv_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        devenv_sink->set_level(spdlog::level::err);
        devenv_sink->set_pattern(fmt::format("[{}] [%^%l%$] %v", vkfw_app::logTag));

        std::shared_ptr<spdlog::sinks::base_sink<std::mutex>> file_sink;
        if constexpr (vkfw_core::debug_build) {
            file_sink = std::make_shared<vkfw_core::spdlog::sinks::rotating_open_file_sink_mt>(
                directory.empty() ? std::string{name} : std::string{directory}.append("/").append(name), 5);
            file_sink->set_level(spdlog::level::trace);
        } else {
            file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                directory.empty() ? std::string{name} : std::string{directory}.append("/").append(name), 5);
            file_sink->set_level(spdlog::level::trace);
        }

        spdlog::sinks_init_list sink_list = {file_sink, console_sink, devenv_sink};
        auto logger = std::make_shared<spdlog::logger>(vkfw_app::logTag.data(), sink_list.begin(), sink_list.end());

        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::err);

        if constexpr (vkfw_core::debug_build) {
            spdlog::set_level(spdlog::level::trace);
        } else {
            spdlog::set_level(spdlog::level::err);
        }

        spdlog::info("Log created.");

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return 0;
    }

    vkfw_app::FWApplication app;

    spdlog::debug("Starting main loop.");
    app.StartRun();
    auto done = false;
    while (app.IsRunning() && !done) {
        try {
            app.Step();
        }
        catch (std::runtime_error e) {
            spdlog::critical("Could not process frame step: {}\nExiting.", e.what());
            done = true;
        }
    }
    app.EndRun();
    spdlog::debug("Main loop ended.");

    return 0;
}
