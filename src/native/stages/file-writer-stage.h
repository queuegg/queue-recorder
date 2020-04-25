#ifndef FILE_WRITER_STAGE_H
#define FILE_WRITER_STAGE_H
#include <stdio.h>

#include "stage.h"
class FileWriterStage : public PipelineStage
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