#ifndef DESKTOP_DUPLICATION_STAGE_H
#define DESKTOP_DUPLICATION_STAGE_H

#include <d3d11.h>
#include <dxgi1_2.h>

#include "stage.h"
#include "common/limiter.h"

class DesktopDuplicationStage : public PipelineStage
{
public:
    void initialize(PipelineConfig *pipelineConfig,
                    PipelineContext *pipelineContext);
    void *process(void *input);
    void shutdown();

    void resume();

private:
    void initializeDuplication();

    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *context = nullptr;
    IDXGIOutputDuplication *duplication = nullptr;
    // Our pointer to the texture from the latest frame capture.
    ID3D11Texture2D *texture = nullptr;

    Limiter *limiter = nullptr;
    unsigned long totalFrameCount = 0;
    unsigned screenId = 0;
    unsigned frameRate = 0;
    bool canReinitialize = true;
    bool captureCursor = false;
};
#endif