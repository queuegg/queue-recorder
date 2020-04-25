#define UNICODE
#include <windows.h>
#include <wingdi.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "gdi-capture-stage.h"
#include "common/d3d11-utils.h"

struct FindMatchingWindowResults
{
    const char *title;
    HWND hwnd;
};

BOOL findMatchingWindow(HWND hwnd, LPARAM lParam)
{
    char title[256];
    GetWindowTextA(hwnd, title, 256);
    FindMatchingWindowResults *results = (FindMatchingWindowResults *)lParam;
    if (strcmp(title, results->title) == 0)
    {
        results->hwnd = hwnd;
        return false;
    }
    return true;
}

/** Requires a complete match. */
HWND getWindowHandleByTitle(std::string &title)
{
    FindMatchingWindowResults results;
    results.title = title.c_str();
    results.hwnd = nullptr;
    EnumWindows(findMatchingWindow, (LPARAM)&results);
    return results.hwnd;
}

void GdiCaptureStage::initialize(PipelineConfig *pipelineConfig,
                                 PipelineContext *pipelineContext)
{

    captureCursor = pipelineConfig->video.captureCursor;

    createDeviceAndContext(&device, &context);

    // Get the window
    if (pipelineConfig->video.windowTitle.empty())
    {
        hWnd = GetForegroundWindow();
    }
    else
    {
        hWnd = getWindowHandleByTitle(pipelineConfig->video.windowTitle);
    }

    if (!hWnd)
    {
        throw std::runtime_error("Could not find window");
    }

    hdcWindow = GetDC(hWnd);

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    width = rcClient.right - rcClient.left;
    height = rcClient.bottom - rcClient.top;

    // Create a new texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
    throwIfFail(device->CreateTexture2D(&desc, NULL, &texture), "createTexture");
    throwIfFail(texture->QueryInterface(__uuidof(IDXGISurface1), reinterpret_cast<void **>(&surface)), "querySurface");

    // pass along everything to the rest of the pipeline
    pipelineContext->d3Device = (void *)device;
    pipelineContext->inputWidth = width;
    pipelineContext->inputHeight = height;

    // Mark the start time so we can aim for our target fps.
    limiter = new Limiter(pipelineConfig->video.frameRate);
}

void *GdiCaptureStage::process(void *input)
{
    // Use the limiter for timing.
    limiter->wait();

    // BitBlt into our texture.
    HDC destHdc;
    surface->GetDC(true, &destHdc);
    if (!BitBlt(destHdc, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY))
    {
        throw std::runtime_error("BitBlt failed");
    }

    if (captureCursor)
    {
        CURSORINFO cursorInfo = {0};
        cursorInfo.cbSize = sizeof(cursorInfo);
        if (GetCursorInfo(&cursorInfo))
        {
            if (cursorInfo.flags == CURSOR_SHOWING)
            {
                ScreenToClient(hWnd, &cursorInfo.ptScreenPos);
                DrawIconEx(destHdc, cursorInfo.ptScreenPos.x, cursorInfo.ptScreenPos.y,
                           cursorInfo.hCursor, 0, 0, 0, 0, DI_NORMAL | DI_DEFAULTSIZE);
            }
        }
    }

    surface->ReleaseDC(nullptr);

    // Pass the texture on to the next pipeline stage.
    return (void *)texture;
}

void GdiCaptureStage::shutdown()
{
    if (limiter)
        delete limiter;
    if (hdcWindow)
        ReleaseDC(hWnd, hdcWindow);
    if (context)
        context->Release();
    if (device)
        device->Release();
    if (texture)
        texture->Release();
}

void GdiCaptureStage::resume()
{
    if (limiter)
        limiter->reset();
}