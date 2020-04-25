#ifndef GDI_CAPTURE_STAGE_H
#define GDI_CAPTURE_STAGE_H
#include "stage.h"

#include "common/limiter.h"

#include "../common.h "

class GdiCaptureStage : public PipelineStage
{
public:
    void initialize(PipelineConfig *pipelineConfig,
                    PipelineContext *pipelineContext);
    void *process(void *input);
    void shutdown();
    void resume();

private:
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *context = nullptr;
    ID3D11Texture2D *texture = nullptr;
    IDXGISurface1 *surface = nullptr;
    unsigned width;
    unsigned height;
    HWND hWnd;
    HDC hdcWindow;

    Limiter *limiter = nullptr;
    bool captureCursor = false;
};
#endif