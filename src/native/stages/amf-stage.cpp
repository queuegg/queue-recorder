#include "../amf/public/include/components/VideoEncoderVCE.h"
#include "../amf/public/common/AMFFactory.h"

#include "amf-stage.h"
#include <d3d11.h>
#include <dxgi1_2.h>

inline void throwIfFailAmd(AMF_RESULT res, const char *prefix)
{
    if (res != 0)
    {
        char buffer[256];
        sprintf_s(buffer, "%s error code=%x", prefix, res);
        std::cout << "Error: " << buffer << std::endl;
        throw std::runtime_error(buffer);
    }
}

void AmfStage::initialize(PipelineConfig *pipelineConfig,
                          PipelineContext *pipelineContext)
{
    unsigned width = pipelineContext->inputWidth;
    unsigned height = pipelineContext->inputHeight;
    frameRate = pipelineConfig->video.frameRate;

    throwIfFailAmd(g_AMFFactory.Init(), "init");
    // Create the context
    throwIfFailAmd(g_AMFFactory.GetFactory()->CreateContext(&context), "context");
    throwIfFailAmd(context->InitDX11(pipelineContext->d3Device), "initDX11");
    throwIfFailAmd(g_AMFFactory.GetFactory()->CreateComponent(context, AMFVideoEncoderVCE_AVC, &encoder), "createEncoder");

    // Configure encoder
    throwIfFailAmd(encoder->SetProperty(AMF_VIDEO_ENCODER_USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCONDING), "setEncoder");
    throwIfFailAmd(encoder->SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE, 5000000), "setBitrate");
    throwIfFailAmd(encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMESIZE, ::AMFConstructSize(width, height)), "setSize");
    throwIfFailAmd(encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, ::AMFConstructRate(frameRate, 1)), "setFramerate");
    // Init the encoder and set up a surface to use as an intermediate placeholder.
    throwIfFailAmd(encoder->Init(amf::AMF_SURFACE_BGRA, width, height), "initEncoder");
    throwIfFailAmd(context->AllocSurface(amf::AMF_MEMORY_DX11, amf::AMF_SURFACE_BGRA, width, height, &surface), "allocSurface");
    // Final buffer wher we store transcoded frames. Probably doesn't need to be this big, but whatever.
    result.rawData = new char[width * height * 4];
}
void *AmfStage::process(void *input)
{
    // Right now the texture needs to be copied to our surface.
    // TODO: Can we use the input texture directly?
    ID3D11DeviceContext *deviceContextDX11 = nullptr;
    ID3D11Device *deviceDX11 = (ID3D11Device *)context->GetDX11Device();                   // no reference counting - do not Release()
    ID3D11Texture2D *surfaceDX11 = (ID3D11Texture2D *)surface->GetPlaneAt(0)->GetNative(); // no reference counting - do not Release()
    deviceDX11->GetImmediateContext(&deviceContextDX11);
    deviceContextDX11->CopyResource(surfaceDX11, (ID3D11Texture2D *)input);
    surface->SetDuration(1000 / frameRate);

    throwIfFailAmd(encoder->SubmitInput(surface), "submitInput");

    amf::AMFDataPtr data;
    encoder->QueryOutput(&data);
    if (data == nullptr)
    {
        // We're probably still waiting for the pipeline to fill up.
        return nullptr;
    }
    // Copy the data out of the buffer into our raw data.
    amf::AMFBufferPtr buffer(data);
    memcpy(result.rawData, buffer->GetNative(), buffer->GetSize());
    result.size = buffer->GetSize();
    return &result;
}

void AmfStage::shutdown()
{
    if (encoder)
    {

        encoder->Terminate();
    }
    if (context)
    {

        context->Terminate();
    }

    g_AMFFactory.Terminate();
    if (result.rawData)
    {

        delete result.rawData;
    }
}

bool AmfStage::isSupported()
{
    auto result = g_AMFFactory.Init();
    g_AMFFactory.Terminate();
    return result == 0;
}