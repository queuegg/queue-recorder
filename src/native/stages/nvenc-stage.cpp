#include <iostream>
#include <stdio.h>

#include "../common.h"
#include "nvenc-stage.h"

bool NvencStage::isSupported()
{
	return NvEncoder::HasDrivers();
}

void *NvencStage::process(void *input)
{
	ID3D11Texture2D *texture = (ID3D11Texture2D *)input;
	const NvEncInputFrame *encoderInput = encoder->GetNextInputFrame();
	ID3D11Texture2D *encoderBuffer = (ID3D11Texture2D *)encoderInput->inputPtr;
	context->CopySubresourceRegion(encoderBuffer, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, texture, 0, NULL);
	encoder->EncodeFrame(packets);

	if (packets.size() == 0)
	{
		return nullptr;
	}

	if (packets.size() > 1)
	{
		throw std::runtime_error("Got more packets than expected");
	}

	std::vector<uint8_t> &packet = packets.at(0);
	results.rawData = packet.data();
	results.size = packet.size();
	return &results;
}

void NvencStage::initialize(PipelineConfig *pipelineConfig,
							PipelineContext *pipelineContext)
{
	// Note that this height/width come directly from the capture surface.
	unsigned width = pipelineContext->inputWidth;
	unsigned height = pipelineContext->inputHeight;
	ID3D11Device *device = (ID3D11Device *)pipelineContext->d3Device;

	device->GetImmediateContext(&context);
	encoder = new NvEncoderD3D11(device, width, height, NV_ENC_BUFFER_FORMAT_ARGB);

	NV_ENC_INITIALIZE_PARAMS encInitParams = {0};
	NV_ENC_CONFIG encConfig = {0};

	ZeroMemory(&encInitParams, sizeof(encInitParams));
	ZeroMemory(&encConfig, sizeof(encConfig));
	encInitParams.encodeConfig = &encConfig;
	encInitParams.encodeWidth = width;
	encInitParams.encodeHeight = height;
	encInitParams.maxEncodeWidth = width;
	encInitParams.maxEncodeHeight = height;

	encoder->CreateDefaultEncoderParams(&encInitParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_LOW_LATENCY_HP_GUID);
	encInitParams.frameRateNum = pipelineConfig->video.frameRate;
	encInitParams.encodeConfig->gopLength = pipelineConfig->video.frameRate * 2;
	encoder->CreateEncoder(&encInitParams);
}
void NvencStage::shutdown()
{
	// TODO: There may be some frames hanging out in the queue. We can get these out with EndEncode, but
	// nobody else is ready to accept them...
	if (encoder)
	{
		encoder->DestroyEncoder();
		delete encoder;
	}
}