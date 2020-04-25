#include <iostream>
#include <thread>

#include "pipeline.h"

#include "stages/desktop-duplication-stage.h"
#include "stages/nvenc-stage.h"
#include "stages/amf-stage.h"
#include "stages/wasapi-stage.h"
#include "stages/wav-writer-stage.h"
#include "stages/file-writer-stage.h"
#include "stages/gdi-capture-stage.h"

void processingThreadMain(std::vector<PipelineStage *> stages,
                          std::atomic<bool> &finished,
                          std::mutex &allowProcessing,
                          std::string &error)
{
    while (!finished)
    {
        auto hasLock = allowProcessing.try_lock();
        if (!hasLock)
        {
            // Suspend the pipeline stages
            for (auto &stage : stages)
            {
                stage->pause();
            }
            // Wait to be allowed to proceed
            allowProcessing.lock();
            // Notify all of the stages that we'll be resuming.
            for (auto &stage : stages)
            {
                stage->resume();
            }
        }

        void *data = nullptr;
        for (auto &stage : stages)
        {
            try
            {
                data = stage->process(data);
            }
            catch (std::exception e)
            {
                // Exceptions are considered unrecoverable. Just log the error, let go of our lock, and
                // bail.
                std::cout << "Pipeline process thread encountered an exception " << e.what() << std::endl;
                allowProcessing.unlock();
                error = e.what();
                return;
            }

            if (data == nullptr)
            {
                // This is either the end of a pipeline or a stage got held up.
                // We don't want to continue processing in this case.
                break;
            }
        }

        // Allow the main thread to potential suspend us for the next iteration.
        allowProcessing.unlock();
    }
};

Pipeline::Pipeline(PipelineConfig pipelineConfig)
{
    config = pipelineConfig;
}

Pipeline::~Pipeline()
{
    for (auto &stage : stages)
    {
        delete stage;
    }
}

PipelineStage *createStage(PipelineStageType stageType)
{
    PipelineStage *stage = nullptr;
    switch (stageType)
    {
    case DESKTOP_DUPLICATION:
        stage = new DesktopDuplicationStage();
        break;
    case NVENC:
        stage = new NvencStage();
        break;
    case WASAPI:
        stage = new WasapiStage();
        break;
    case WAV_WRITER:
        stage = new WavWriterStage();
        break;
    case FILE_WRITER:
        stage = new FileWriterStage();
        break;
    case AMF:
        stage = new AmfStage();
        break;
    case GDI_CAPTURE:
        stage = new GdiCaptureStage();
        break;
    }
    return stage;
}

void Pipeline::addStage(PipelineStageType stageType)
{
    PipelineStage *stage = createStage(stageType);
    stages.push_back(stage);
};

bool Pipeline::supportsStage(PipelineStageType stageType)
{
    PipelineStage *stage = createStage(stageType);
    bool isSupported = stage->isSupported();
    delete stage;
    return isSupported;
}

void Pipeline::initialize()
{
    if (initialized)
        return;
    PipelineContext context;
    for (auto &stage : stages)
    {
        // Note that initialize can throw, so the caller should be prepared to handle that.
        stage->initialize(&config, &context);
    }
    initialized = true;
}

void Pipeline::start()
{
    initialize();
    // Start the processing in a new thread.
    finished = false;
    processingThread = new std::thread(processingThreadMain, stages,
                                       std::ref(finished), std::ref(allowProcessing),
                                       std::ref(processingError));
};

void Pipeline::pause()
{
    allowProcessing.lock();
    paused = true;
}

void Pipeline::resume()
{
    paused = false;
    allowProcessing.unlock();
}

void Pipeline::stop()
{
    if (paused)
    {
        allowProcessing.unlock();
    }

    finished = true;
    if (processingThread)
    {
        processingThread->join();
        delete processingThread;
    }

    for (auto &stage : stages)
    {
        stage->shutdown();
    }
};

std::vector<std::string> Pipeline::pollErrors()
{
    std::vector<std::string> errors;
    if (processingError.size())
    {
        errors.push_back(processingError);
        processingError.clear();
    }
    return errors;
}