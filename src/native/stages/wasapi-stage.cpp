#include <iostream>
#include <stdexcept>

#include "../common.h"
#include "wasapi-stage.h"

const unsigned MAX_BUFFER_SIZE = 32768;

void WasapiStage::initialize(PipelineConfig *pipelineConfig,
                             PipelineContext *pipelineContext)
{
    HRESULT hr = S_OK;

    CoInitialize(nullptr);

    // Get the default audio device
    IMMDeviceEnumerator *deviceEnumerator;
    CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void **)&deviceEnumerator);
    deviceEnumerator->GetDefaultAudioEndpoint(pipelineConfig->audio.render ? eRender : eCapture, eConsole, &device);

    // Now we make an audio client
    device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **)&audioClient);

    // Get the periodicity and format
    audioClient->GetMixFormat(&wfx);

    // Coerce into 16 bit for the time being.
    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_IEEE_FLOAT:
        wfx->wFormatTag = WAVE_FORMAT_PCM;
        wfx->wBitsPerSample = 16;
        wfx->nBlockAlign = wfx->nChannels * wfx->wBitsPerSample / 8;
        wfx->nAvgBytesPerSec = wfx->nBlockAlign * wfx->nSamplesPerSec;
        break;
    case WAVE_FORMAT_EXTENSIBLE:
        PWAVEFORMATEXTENSIBLE ex = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(wfx);
        if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, ex->SubFormat))
        {
            ex->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            ex->Samples.wValidBitsPerSample = 16;
            wfx->wBitsPerSample = 16;
            wfx->nBlockAlign = wfx->nChannels * wfx->wBitsPerSample / 8;
            wfx->nAvgBytesPerSec = wfx->nBlockAlign * wfx->nSamplesPerSec;
        }
        break;
    }

    audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                            pipelineConfig->audio.render ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0, 0, 0, wfx, 0);

    // Get the capture client
    audioClient->GetService(__uuidof(IAudioCaptureClient), (void **)&audioCaptureClient);
    audioClient->Start();

    // Make the buffer big enough to hold bufferSize frames.
    buffer = new char[wfx->nBlockAlign * bufferSize];

    // Set up our timer
    wakeUpHandle = CreateWaitableTimer(nullptr, false, nullptr);
    REFERENCE_TIME devicePeriod;
    hr = audioClient->GetDevicePeriod(&devicePeriod, NULL);
    LARGE_INTEGER firstFire;
    firstFire.QuadPart = -devicePeriod / 2;
    long timeBetweenFires = devicePeriod / 2 / (10 * 1000);
    SetWaitableTimer(wakeUpHandle, &firstFire, timeBetweenFires, nullptr, nullptr, false);

    // Set up the context
    pipelineContext->samplesPerSecond = wfx->nSamplesPerSec;
    pipelineContext->channels = wfx->nChannels;
    pipelineContext->bitsPerSample = wfx->wBitsPerSample;

    // If we are trying to capture the audio render path (loopback, everything coming out
    // of the speakers), we need to do some extra work. Essentially, WASAPI won't deliver
    // updates when there is no data. This means that if there is silence we won't get
    // any data and there will be timestamp issues (the audio will be shorter than the video).
    // To work around this we set up an AudioRenderClient that writes a consistant silent
    // sample. This means we'll always have something to record.
    if (pipelineConfig->audio.render)
    {
        UINT32 frames;
        LPBYTE buffer;
        device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **)&renderClient);
        renderClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, wfx, 0);
        throwIfFail(renderClient->GetBufferSize(&frames), "render:getBufferSize");
        throwIfFail(renderClient->GetService(__uuidof(IAudioRenderClient), (void **)&renderer), "render:getService");
        throwIfFail(renderer->GetBuffer(frames, &buffer), "renderer:getBuffer");
        memset(buffer, 0, frames * wfx->nBlockAlign);
        renderer->ReleaseBuffer(frames, 0);
        renderClient->Start();
    }
};

void *WasapiStage::process(void *data)
{
    UINT32 nextPacketSize;
    audioCaptureClient->GetNextPacketSize(&nextPacketSize);

    // If we don't have any data, wait for some more to show up in the queue.
    if (!nextPacketSize)
    {
        WaitForSingleObject(wakeUpHandle, INFINITE);
    }

    audioCaptureClient->GetNextPacketSize(&nextPacketSize);
    if (!nextPacketSize)
    {
        return nullptr;
    }

    BYTE *captureBuffer;
    UINT32 numFramesToRead;
    DWORD flags;

    audioCaptureClient->GetBuffer(&captureBuffer, &numFramesToRead, &flags, nullptr, nullptr);

    while (numFramesToRead > bufferSize)
    {
        bufferSize = bufferSize * 2;
        if (bufferSize > MAX_BUFFER_SIZE)
        {
            std::cout << "Num frames exceeded MAX_BUFFER_SIZE " << numFramesToRead << std::endl;
            throw std::runtime_error("Audio frames overlowed buffer.");
        }
        std::cout << "Got too many audio frames, resizing to " << bufferSize << std::endl;
        if (buffer)
        {
            delete buffer;
        }
        buffer = new char[wfx->nBlockAlign * bufferSize];
    }

    memcpy(buffer, captureBuffer, numFramesToRead * wfx->nBlockAlign);

    audioCaptureClient->ReleaseBuffer(numFramesToRead);

    results.rawData = buffer;
    results.size = numFramesToRead * wfx->nBlockAlign;
    return &results;
};

void WasapiStage::shutdown()
{
    if (buffer)
    {
        delete buffer;
    }
};