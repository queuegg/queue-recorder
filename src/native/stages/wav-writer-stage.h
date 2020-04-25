#ifndef WAV_WRITER_STAGE_H
#define WAV_WRITER_STAGE_H
#include <stdio.h>

#include "stage.h"

class WavWriterStage : public PipelineStage
{
public:
    void initialize(PipelineConfig *pipelineConfig,
                    PipelineContext *pipelineContext);
    void *process(void *input);
    void shutdown();

private:
    FILE *file = nullptr;
};
#endif