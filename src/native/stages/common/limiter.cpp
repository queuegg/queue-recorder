#include <iostream>
#include "limiter.h"

Limiter::Limiter(unsigned targetFrequency)
{
    frequency = targetFrequency;
    QueryPerformanceFrequency(&performanceFrequency);
}

void Limiter::wait()
{
    long long wait = getWait();
    if (wait > 0)
    {
        Sleep(wait);
    }
    calls++;
}

long long Limiter::getWait()
{
    if (startTime.QuadPart == 0)
    {
        QueryPerformanceCounter(&startTime);
    }
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    auto elapsedMs = (calls * 1000) / frequency;
    long long nextCallTime = startTime.QuadPart + elapsedMs * (performanceFrequency.QuadPart / 1000);
    long long wait = (nextCallTime - currentTime.QuadPart) / (performanceFrequency.QuadPart / 1000);
    return wait > 0 ? wait : 0;
}

void Limiter::reset()
{
    calls = 0;
    QueryPerformanceCounter(&startTime);
}