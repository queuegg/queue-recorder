#ifndef COMMON_H
#define COMMON_H

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dxgi.lib")

#include <iostream>

#include <comdef.h>

inline void throwIfFail(HRESULT hr, const char *prefix)
{
	if (FAILED(hr))
	{
		char buffer[256];
		sprintf_s(buffer, "%s error code=%x", prefix, hr);
		std::cout << "Error: " << buffer << std::endl;
		throw std::runtime_error(buffer);
	}
}

struct DataAndSize
{
	void *rawData;
	unsigned size;
};

#endif