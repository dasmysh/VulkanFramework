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
#include <minwindef.h>
#include <g3log/logworker.hpp>


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    const std::string directory = "./";
    const std::string name = vkuapp::logFileName;
    auto worker = g3::LogWorker::createLogWorker();
    auto handle = worker->addDefaultLogger(name, directory);

    LOG(INFO) << L"Log created.";

    vkuapp::FWApplication app;

    LOG(DEBUG) << L"Starting main loop.";
    app.StartRun();
    auto done = false;
    while (app.IsRunning() && !done) {
        try {
            app.Step();
        }
        catch (std::runtime_error e) {
            LOG(FATAL) << L"Could not process frame step: " << e.what() << std::endl << L"Exiting.";
            done = true;
        }
    }
    app.EndRun();
    LOG(DEBUG) << L"Main loop ended.";

    return 0;
}
