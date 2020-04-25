#ifndef AMF_STAGE_H
#define AMF_STAGE_H
#include "stage.h"

#include "../amf/public/common/AMFFactory.h"

#include "../common.h "

class AmfStage : public PipelineStage
{
public:
    void initialize(PipelineConfig *pipelineConfig,
                    PipelineContext *pipelineContext);
    void *process(void *input);
    void shutdown();
    bool isSupported();

private:
    amf::AMFComponentPtr encoder = nullptr;
    amf::AMFContextPtr context;
    amf::AMFSurfacePtr surface = nullptr;
    DataAndSize result;
    unsigned frameRate;
};
#endif