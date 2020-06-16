#include <iostream>

#include <napi.h>

#include "pipeline.h"
#include "nvenc/NvEncoder.h"

class PipelineWrapper : public Napi::ObjectWrap<PipelineWrapper>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    PipelineWrapper(const Napi::CallbackInfo &info);
    ~PipelineWrapper();

private:
    static Napi::FunctionReference constructor;
    Pipeline *pipeline;
    void addStage(const Napi::CallbackInfo &info);
    void initialize(const Napi::CallbackInfo &info);
    void start(const Napi::CallbackInfo &info);
    void pause(const Napi::CallbackInfo &info);
    void resume(const Napi::CallbackInfo &info);
    void stop(const Napi::CallbackInfo &info);
    Napi::Value supportsStage(const Napi::CallbackInfo &info);
    Napi::Value pollErrors(const Napi::CallbackInfo &info);
};

Napi::FunctionReference PipelineWrapper::constructor;

Napi::Object PipelineWrapper::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env, "Pipeline", {
                                                           InstanceMethod("addStage", &PipelineWrapper::addStage),
                                                           InstanceMethod("initialize", &PipelineWrapper::initialize),
                                                           InstanceMethod("start", &PipelineWrapper::start),
                                                           InstanceMethod("stop", &PipelineWrapper::stop),
                                                           InstanceMethod("pause", &PipelineWrapper::pause),
                                                           InstanceMethod("resume", &PipelineWrapper::resume),
                                                           InstanceMethod("pollErrors", &PipelineWrapper::pollErrors),
                                                           InstanceMethod("supportsStage", &PipelineWrapper::supportsStage),
                                                       });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Pipeline", func);
    return exports;
};

PipelineWrapper::PipelineWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<PipelineWrapper>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "Pipeline config must be an object").ThrowAsJavaScriptException();
    }

    PipelineConfig config;
    // Fill out the configuration from the input configuration
    Napi::Object configObject = info[0].As<Napi::Object>();

    if (configObject.Has("video"))
    {
        auto videoConfig = configObject.Get("video").As<Napi::Object>();
        if (videoConfig.Has("frameRate"))
        {
            config.video.frameRate = videoConfig.Get("frameRate").As<Napi::Number>();
        }

        if (videoConfig.Has("captureCursor"))
        {
            config.video.captureCursor = videoConfig.Get("captureCursor").As<Napi::Boolean>();
        }

        if (videoConfig.Has("source"))
        {
            auto sourceConfig = videoConfig.Get("source").As<Napi::Object>();
            if (sourceConfig.Has("screenId"))
            {
                config.video.screenId = sourceConfig.Get("screenId").As<Napi::Number>();
            }
            if (sourceConfig.Has("windowTitle"))
            {
                config.video.windowTitle = std::string(sourceConfig.Get("windowTitle").As<Napi::String>());
            }
        }
    }

    if (configObject.Has("audio"))
    {
        auto audioConfig = configObject.Get("audio").As<Napi::Object>();
        if (audioConfig.Has("source"))
        {
            auto audioSource = audioConfig.Get("source").As<Napi::Object>();
            if (audioSource.Has("type"))
            {
                auto audioSourceType = audioSource.Get("type").As<Napi::String>();
                if (std::string(audioSourceType) == "render")
                {
                    config.audio.render = true;
                }
                else if (std::string(audioSourceType) == "capture")
                {
                    config.audio.render = false;
                    std::cout << "Will try to do a capture" << std::endl;
                }
            }
        }
    }

    if (configObject.Has("output"))
    {
        auto outputConfig = configObject.Get("output").As<Napi::Object>();
        if (outputConfig.Has("fileName"))
        {
            config.output.fileName = std::string(outputConfig.Get("fileName").As<Napi::String>());
        }
    }

    // Output is the only field that doesn't have a default value.
    // TODO: Not every pipeline needs an output... right?
    if (config.output.fileName.size() == 0)
    {
        Napi::TypeError::New(env, "Pipeline could not determine a suitable output").ThrowAsJavaScriptException();
    }

    pipeline = new Pipeline(config);
};

PipelineWrapper::~PipelineWrapper()
{
    delete pipeline;
};

PipelineStageType getStageTypeFromString(std::string &stageType, Napi::Env &env)
{
    if (std::string(stageType) == "DESKTOP_DUPLICATION")
    {
        return DESKTOP_DUPLICATION;
    }
    else if (std::string(stageType) == "NVENC")
    {
        return NVENC;
    }
    else if (std::string(stageType) == "WASAPI")
    {
        return WASAPI;
    }
    else if (std::string(stageType) == "WAV_WRITER")
    {
        return WAV_WRITER;
    }
    else if (std::string(stageType) == "FILE_WRITER")
    {
        return FILE_WRITER;
    }
    else if (std::string(stageType) == "AMF")
    {
        return AMF;
    }
    else if (std::string(stageType) == "GDI_CAPTURE")
    {
        return GDI_CAPTURE;
    }
    else
    {
        Napi::TypeError::New(env, "Unknown stage type").ThrowAsJavaScriptException();
    }
}
void PipelineWrapper::addStage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected string").ThrowAsJavaScriptException();
    }

    Napi::String stageType = info[0].As<Napi::String>();
    pipeline->addStage(getStageTypeFromString(std::string(stageType), env));
};

void PipelineWrapper::initialize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    try
    {
        pipeline->initialize();
    }
    catch (std::exception e)
    {
        Napi::Error::New(env, "Failed to initialize pipeline: " + std::string(e.what())).ThrowAsJavaScriptException();
    }
}

void PipelineWrapper::start(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    try
    {
        pipeline->start();
    }
    catch (NVENCException e)
    {
        Napi::Error::New(env, "Failed to start pipeline: " + std::string(e.what())).ThrowAsJavaScriptException();
    }
    catch (std::exception e)
    {
        Napi::Error::New(env, "Failed to start pipeline: " + std::string(e.what())).ThrowAsJavaScriptException();
    }
};

void PipelineWrapper::pause(const Napi::CallbackInfo &info)
{
    pipeline->pause();
};

void PipelineWrapper::resume(const Napi::CallbackInfo &info)
{
    pipeline->resume();
}

void PipelineWrapper::stop(const Napi::CallbackInfo &info)
{
    pipeline->stop();
};

Napi::Value PipelineWrapper::pollErrors(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    auto pipelineErrors = pipeline->pollErrors();
    Napi::Array errors = Napi::Array::New(env, pipelineErrors.size());
    for (unsigned i = 0; i < pipelineErrors.size(); i++)
    {
        errors[i] = Napi::String::New(env, pipelineErrors[i]);
    }
    return errors;
};

Napi::Value PipelineWrapper::supportsStage(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected string").ThrowAsJavaScriptException();
    }

    Napi::String stageType = info[0].As<Napi::String>();
    bool result = pipeline->supportsStage(getStageTypeFromString(std::string(stageType), env));
    Napi::Boolean res = Napi::Boolean::New(env, result);
    return res;
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    PipelineWrapper::Init(env, exports);
    return exports;
};

NODE_API_MODULE(screenCapture, Init);