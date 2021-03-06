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
#include <g3log/logworker.hpp>
#include <g3log/loglevels.hpp>
#ifndef NDEBUG
#include <core/g3log/rotfilesink.h>
#else
#include <core/g3log/filesink.h>
#endif

#pragma warning(push, 3)
#include <Windows.h>
#pragma warning(pop)



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    const std::string directory = "./";
    const std::string name = vkuapp::logFileName;
    auto worker = g3::LogWorker::createLogWorker();
#ifndef NDEBUG
    auto handle = worker->addSink(std::make_unique<vku::RotationFileSink>(name, directory, 5), &vku::RotationFileSink::fileWrite);
#else
    auto handle = worker->addSink(std::make_unique<vku::FileSink>(name, directory, false), &vku::FileSink::fileWrite);
#endif

    g3::only_change_at_initialization::addLogLevel(G3LOG_DEBUG, false);
    g3::only_change_at_initialization::addLogLevel(WARNING, false);
    g3::only_change_at_initialization::addLogLevel(VK_GEN, true);
    g3::only_change_at_initialization::addLogLevel(VK_INFO, true);
    g3::only_change_at_initialization::addLogLevel(VK_ERROR, true);

#ifndef NDEBUG
    g3::only_change_at_initialization::addLogLevel(G3LOG_DEBUG, true);
    g3::only_change_at_initialization::addLogLevel(WARNING, true);
    g3::only_change_at_initialization::addLogLevel(VK_DEBUG, true);
    g3::only_change_at_initialization::addLogLevel(VK_WARNING, true);
    g3::only_change_at_initialization::addLogLevel(VK_PERF_WARNING, true);
#endif

    g3::initializeLogging(worker.get());

    LOG(INFO) << "Log created.";

    vkuapp::FWApplication app;

    LOG(G3LOG_DEBUG) << "Starting main loop.";
    app.StartRun();
    auto done = false;
    while (app.IsRunning() && !done) {
        try {
            app.Step();
        }
        catch (std::runtime_error e) {
            LOG(FATAL) << "Could not process frame step: " << e.what() << std::endl << "Exiting.";
            done = true;
        }
    }
    app.EndRun();
    LOG(G3LOG_DEBUG) << "Main loop ended.";

    return 0;
}
