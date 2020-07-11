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

void sleep_ms(int64_t ms)
{
    if (ms > 0)
    {
        std::mutex *expire_queue_lock = wait_expire(now_ms() + ms);
        set_call_after_sched_back([expire_queue_lock] () {
            expire_queue_lock->unlock();
        });
        switch_to_worker_sched();
    }
}

}
