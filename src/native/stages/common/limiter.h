#ifndef LIMITER_H
#define LIMITER_H

#include <windows.h>

class Limiter
{
public:
    /**
     * Creates a new limiter. targetFrequency should be how often 
     * the limited item should be invoked per second.
     */
    Limiter(unsigned targetFrequency);

    /** 
     * Waits until the next time that this limiter is valid. 
     * Note that this function is blocking.
     */
    void wait();

    /** Gets the current wait from the limiter. */
    long long getWait();

    /** Reset any limits (useful if some time has passed that the limiter should not care about). */
    void reset();

private:
    unsigned frequency;
    unsigned calls = 0;
    LARGE_INTEGER startTime = {0};
    LARGE_INTEGER performanceFrequency;
};

#endif