#include "../common.h"
#include "file-writer-stage.h"

void FileWriterStage::initialize(PipelineConfig *pipelineConfig,
                                 PipelineContext *pipelineContext)
{
    unsigned opened = fopen_s(&file, pipelineConfig->output.fileName.c_str(), "wb");
    if (opened != 0)
    {
        throw std::runtime_error("Failed to open file, error code=" + std::to_string(opened));
    }
}
void *FileWriterStage::process(void *data)
{
    unsigned size = ((DataAndSize *)data)->size;
    void *rawData = ((DataAndSize *)data)->rawData;

    fwrite(rawData, 1, size, file);
    return nullptr;
}
void FileWriterStage::shutdown()
{
    if (file)
    {
        fflush(file);
        fclose(file);
    }
}