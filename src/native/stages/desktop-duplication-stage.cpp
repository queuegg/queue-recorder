#include "../common.h"

#include "desktop-duplication-stage.h"
#include "common/d3d11-utils.h"

void *DesktopDuplicationStage::process(void *input)
{
	if (!duplication)
	{
		// Normally we should have a duplication, but during screen size changes we may not.
		limiter->wait();
		initializeDuplication();
		if (!duplication)
		{
			totalFrameCount++;
			return texture; // Return the most recent frame we were able to capture.
		}
	}

	IDXGIResource *resource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;

	HRESULT hr = duplication->AcquireNextFrame(limiter->getWait(), &frameInfo, &resource);
	totalFrameCount++;

	// Sometimes we can get frames a bit too fast, so we just need to take a breather.
	limiter->wait();

	if (FAILED(hr))
	{
		// If we got a timeout that's fine, we'll return our most recent texture and hang out for a minute.
		if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		{
			return texture;
		}
		else if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL)
		{
			// We've lost access to our duplication output. This can happen when there is a screen change
			// (like alt-tab). To handle this, we'll hold on to our latest texture until we can reacquire
			// the output.
			duplication->Release();
			duplication = nullptr;
			return texture;
		}
		else
		{
			throwIfFail(hr, "AcquireNextFrame");
		}
	}

	// If we get at least one frame you can try to reinit the next time you hit an error.
	canReinitialize = true;

	ID3D11Texture2D *frameTexture;
	throwIfFail(resource->QueryInterface(IID_PPV_ARGS(&frameTexture)), "Query texture");

	// Create our middle man texture if necessary
	if (!texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		frameTexture->GetDesc(&desc);
		// Set up some flags to make sure this will work with the rest of the pipeline.
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.ArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

		throwIfFail(device->CreateTexture2D(&desc, nullptr, &texture), "Create texture");
	}

	// Copy out of the frame texture into our own.
	context->CopyResource(texture, frameTexture);

	// Now that we've gotten a frame we can release it from the duplication.
	duplication->ReleaseFrame();

	// Apply the cursor if necessary
	if (captureCursor)
	{
		CURSORINFO cursorInfo = {0};
		cursorInfo.cbSize = sizeof(cursorInfo);
		if (GetCursorInfo(&cursorInfo))
		{
			if (cursorInfo.flags == CURSOR_SHOWING)
			{
				IDXGISurface1 *surface;
				auto hr = texture->QueryInterface(IID_PPV_ARGS(&surface));
				if (!FAILED(hr))
				{
					HDC hdc;
					surface->GetDC(FALSE, &hdc);
					DrawIconEx(hdc, cursorInfo.ptScreenPos.x, cursorInfo.ptScreenPos.y,
							   cursorInfo.hCursor, 0, 0, 0, 0, DI_NORMAL | DI_DEFAULTSIZE);
					surface->ReleaseDC(nullptr);
				}
				surface->Release();
			}
		}
	}

	return texture;
}

void DesktopDuplicationStage::initialize(PipelineConfig *pipelineConfig,
										 PipelineContext *pipelineContext)
{
	screenId = pipelineConfig->video.screenId;
	frameRate = pipelineConfig->video.frameRate;
	captureCursor = pipelineConfig->video.captureCursor;
	if (frameRate == 0)
	{
		throw std::runtime_error("Frame rate must be greater than 0");
	}

	createDeviceAndContext(&device, &context);

	initializeDuplication();

	DXGI_OUTDUPL_DESC duplDesc;
	duplication->GetDesc(&duplDesc);

	// Store the things that are needed in the context
	pipelineContext->d3Device = (void *)device;
	pipelineContext->inputWidth = duplDesc.ModeDesc.Width;
	pipelineContext->inputHeight = duplDesc.ModeDesc.Height;

	// Mark the start time so we can aim for our target fps.
	limiter = new Limiter(frameRate);
}

void DesktopDuplicationStage::shutdown()
{
	if (context)
		context->Release();
	if (device)
		device->Release();
	if (duplication)
		duplication->Release();
	if (limiter)
		delete limiter;

	std::cout << "Captured a total of " << totalFrameCount << " frames" << std::endl;
}

void DesktopDuplicationStage::initializeDuplication()
{
	// DXGI components
	IDXGIDevice *DxgiDevice = nullptr;
	IDXGIAdapter *DxgiAdapter = nullptr;
	IDXGIOutput *DxgiOutput = nullptr;
	IDXGIOutput1 *DxgiOutput1 = nullptr;

	// The next step is to the get the DXGI output. Right now, we just read the default
	// one from the D3D11 device.
	throwIfFail(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&DxgiDevice)), "Query dxgi");
	throwIfFail(DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(&DxgiAdapter)), "Get dxgi parent");
	throwIfFail(DxgiAdapter->EnumOutputs(screenId, &DxgiOutput), "Enum outputs");
	throwIfFail(DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void **>(&DxgiOutput1)), "Query dxgi output1");
	// TODO: Can we use DuplicateOutput1 to improve performance?
	auto hr = DxgiOutput1->DuplicateOutput(DxgiDevice, &duplication);
	// E_ACCESSDENIED and DXGI_ERROR_UNSUPPORTED are considered retriable errors.
	if (hr != E_ACCESSDENIED && hr != DXGI_ERROR_UNSUPPORTED)
	{
		throwIfFail(hr, "Duplicate Output");
	}

	// Release all of the DXGI devices we no longer need.
	DxgiDevice->Release();
	DxgiAdapter->Release();
	DxgiOutput->Release();
	DxgiOutput1->Release();
}

void DesktopDuplicationStage::resume()
{
	if (limiter)
		limiter->reset();
}