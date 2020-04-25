#ifndef NVENC_STAGE_H
#define NVENC_STAGE_H
#include "stage.h"

#include "../common.h "
#include "../nvenc/NvEncoderD3D11.h"

class NvencStage : public PipelineStage
{
public:
    void initialize(PipelineConfig *pipelineConfig,
                    PipelineContext *pipelineContext);
    void *process(void *input);
    void shutdown();
    bool isSupported();

private:
    NvEncoderD3D11 *encoder = nullptr;
    ID3D11DeviceContext *context;
    std::vector<std::vector<uint8_t>> packets;
    DataAndSize results;
};
#endif