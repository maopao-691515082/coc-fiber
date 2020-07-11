#include <coc_fiber.h>
#include "internal.h"

namespace coc_fiber
{

int64_t now_ms()
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    return (int64_t)now.tv_sec * 1000 + (int64_t)now.tv_usec / 1000;
}

void assert(bool cond)
{
    if (!cond)
    {
        abort();
    }
}

}
