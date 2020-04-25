#include <exception>
#include <iostream>
#include <sstream>
#include <vector>
#include <dxgi1_2.h>

#include "../../common.h"

#include "d3d11-utils.h"

// These are intended to force the graphics card to use hardware graphics adapters.
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

const D3D_FEATURE_LEVEL FEATURE_LEVELS[] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_1,
};
const UINT NUM_FEATURE_LEVELS = ARRAYSIZE(FEATURE_LEVELS);
const UINT CREATION_FLAGS = 0; // D3D11_CREATE_DEVICE_DEBUG;

/**
 * Gets the default device (attached to the default monitor output). 
 */
void createDefaultDeviceAndContext(ID3D11Device **device, ID3D11DeviceContext **context)
{
    throwIfFail(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CREATION_FLAGS, FEATURE_LEVELS, NUM_FEATURE_LEVELS,
                                  D3D11_SDK_VERSION, device, nullptr, context),
                "D3D11CreateDevice");
}

/**
 * Creates a device + context that aligns with a specific device string. This is useful
 * in cases where we _need_ the NVIDIA or AMD device to handle transcoding.
 */
void createSpecificDeviceAndContext(ID3D11Device **device, ID3D11DeviceContext **context,
                                    std::vector<std::wstring> searchStrings)
{
    IDXGIFactory1 *factory;
    IDXGIAdapter *adapter = nullptr;
    throwIfFail(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&factory), "createDxgiFactory");
    UINT i = 0;
    bool found = false;
    while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC adapterDesc;
        adapter->GetDesc(&adapterDesc);
        found = false;
        for (auto &x : searchStrings)
        {
            if (std::wstring(adapterDesc.Description).find(x) != std::wstring::npos)
            {
                found = true;
            }
        }
        if (found)
        {
            break;
        }

        adapter->Release();
        adapter = nullptr;
        i++;
    }

    factory->Release();

    if (adapter != nullptr)
    {
        throwIfFail(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, CREATION_FLAGS, FEATURE_LEVELS, NUM_FEATURE_LEVELS,
                                      D3D11_SDK_VERSION, device, nullptr, context),
                    "D3D11CreateDevice(specific)");
        adapter->Release();
    }
    else
    {
        // We failed to find the adapter we were looking for, but might as well try the default.
        createDefaultDeviceAndContext(device, context);
    }
}

void createDeviceAndContext(ID3D11Device **device, ID3D11DeviceContext **context)
{
    std::vector<std::wstring> searchStrings;
    searchStrings.push_back(L"NVIDIA");
    searchStrings.push_back(L"AMD");
    createSpecificDeviceAndContext(device, context, searchStrings);
}