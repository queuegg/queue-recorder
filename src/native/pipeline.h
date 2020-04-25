#ifndef PIPELINE_H
#define PIPELINE_H
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

#include "stages/stage.h"
#include "pipeline-config.h"

enum PipelineStageType
{
    DESKTOP_DUPLICATION,
    NVENC,
    WASAPI,
    WAV_WRITER,
    FILE_WRITER,
    AMF,
    GDI_CAPTURE
};

class Pipeline
{
public:
    Pipeline(PipelineConfig config);
    ~Pipeline();
    void addStage(PipelineStageType stageType);
    bool supportsStage(PipelineStageType stageType);
    void initialize();
    void start();
    void pause();
    void resume();
    void stop();
    std::vector<std::string> pollErrors();
private:
    bool initialized = false;

    std::atomic<bool> finished;
    std::string processingError;
    // If the pipeline needs to be paused it will acquire the
    // allow processing lock and the processing thread will block
    // until released.
    std::mutex allowProcessing;
    bool paused;

    std::thread *processingThread = nullptr;
    std::vector<PipelineStage *> stages;
    PipelineConfig config;
};
#endif