#ifndef PIPELINE_CONFIG_H
#define PIPELINE_CONFIG_H
#include <string>

/**
 * The pipleline config is specified by the client and should not change
 * over the course of the execution.
 */
struct PipelineVideoConfig
{
    unsigned frameRate = 30;
    unsigned screenId = 0;
    std::string windowTitle;
    bool captureCursor = false;
};

struct PipelineAudioConfig
{
    // Capture the audio rendering. If this is false, we'll instead capture
    // the audio input.
    bool render = true;
};

struct PipelineOutputConfig
{
    std::string fileName;
};

struct PipelineConfig
{
    PipelineOutputConfig output;
    PipelineAudioConfig audio;
    PipelineVideoConfig video;
};

/**
 * The pipeline context is determined by individual stages and conveys information
 * that will not change over the course of execution. This includes things like
 * the audio sample rate, or the D3D11 context being used.
 */
struct PipelineContext
{
    // D3D11 Device. Used to ensure efficent communication between a DesktopDuplication stage
    // and a NVENC stage.
    void *d3Device;

    // Input video configuration
    unsigned inputHeight;
    unsigned inputWidth;

    // Audio information. This meta data is determined at initialization by the WASAPI stage
    // and is recorded by the WAV writer stage.
    unsigned samplesPerSecond;
    unsigned channels;
    unsigned bitsPerSample;
};
#endif