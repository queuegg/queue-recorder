#ifndef WASAPI_STAGE_H
#define WASAPI_STAGE_H
#include <mmdeviceapi.h>
#include <audioclient.h>

#include "stage.h"
#include "../common.h"

class WasapiStage : public PipelineStage
{
public:
    void initialize(PipelineConfig *pipelineConfig,
                    PipelineContext *pipelineContext);
    void *process(void *input);
    void shutdown();

private:
    IAudioCaptureClient *audioCaptureClient;
    IAudioClient *audioClient;
    IAudioClient *renderClient;
    IAudioRenderClient *renderer;
    IMMDevice *device;
    WAVEFORMATEX *wfx;
    void *buffer = nullptr;
    unsigned bufferSize = 1024;

    HANDLE wakeUpHandle;
    DataAndSize results;
};
#endif