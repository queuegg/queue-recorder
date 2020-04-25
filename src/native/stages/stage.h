#ifndef STAGE_H
#define STAGE_H
#include "../pipeline-config.h"

class PipelineStage
{
public:
    /**
     * Initialize the stage. Recieves both a static config, and context
     * that can be filled in by the other stages.
     */
    virtual void initialize(PipelineConfig *pipelineConfig,
                            PipelineContext *pipelineContext) = 0;
    /**
     * Process the pipeline. Data is fed in via a void * and should be outputed
     * in whatever format is expected by the next stage. Note that input should
     * be read only. The previous stage will maintain ownership over the underlying
     * data.
     */
    virtual void *process(void *input) = 0;

    /**
     * Shutdown this stage and release all resources.
     */
    virtual void shutdown() = 0;

    /**
     * Pause the stage. Can be resumed later.
     */
    virtual void pause(){};

    /**
     * Resume this stage from a paused state.
     */
    virtual void resume(){};

    /**
     * Determine if this stage is supported by the underlying hardware.
     */
    virtual bool isSupported() { return true; };
};
#endif