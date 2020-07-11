#include <coc_fiber.h>
#include "internal.h"

namespace coc_fiber
{

static const size_t WORKER_COUNT = 50;

static int worker_event_fds[WORKER_COUNT];

static std::mutex fiber_queue_lock;
static std::list<Fiber *> fiber_queue;

thread_local Fiber *curr_fiber;
thread_local ucontext thread_main_sched_ctx;
thread_local std::function<void ()> call_after_sched_back;

static void do_nothing()
{
}

static Fiber *get_one_fiber()
{
    std::lock_guard<std::mutex> fiber_queue_lock_guard(fiber_queue_lock);
    Fiber *fiber = nullptr;
    if (!fiber_queue.empty())
    {
        fiber = fiber_queue.front();
        fiber_queue.pop_front();
    }
    return fiber;
}

static void worker_thread_main(size_t idx)
{
    int ev_fd = worker_event_fds[idx];
    for (;;)
    {
        Fiber *fiber = get_one_fiber();
        if (fiber == nullptr)
        {
            uint64_t ev_count;
            assert(read(ev_fd, &ev_count, sizeof(ev_count)) != -1);
            continue;
        }

        curr_fiber = fiber;
        call_after_sched_back = do_nothing;
        assert(swapcontext(&thread_main_sched_ctx, &curr_fiber->ctx) == 0);
        call_after_sched_back();
        curr_fiber = nullptr;

        if (fiber->finished())
        {
            delete fiber;
        }
    }
}

void init_workers()
{
    for (size_t i = 0; i < WORKER_COUNT; ++ i)
    {
        worker_event_fds[i] = eventfd(0, 0);
        assert(worker_event_fds[i] != -1);
        std::thread(worker_thread_main, i).detach();
    }
}

void deliver_fiber_to_workers(std::list<Fiber *> fibers)
{
    {
        std::lock_guard<std::mutex> fiber_queue_lock_guard(fiber_queue_lock);
        for (auto iter = fibers.begin(); iter != fibers.end(); ++ iter)
        {
            fiber_queue.push_back(*iter);
        }
    }
    for (size_t i = 0; i < WORKER_COUNT; ++ i)
    {
        uint64_t count = 1;
        assert(write(worker_event_fds[i], &count, sizeof(count)) != -1);
    }
}

void set_call_after_sched_back(std::function<void ()> call)
{
    call_after_sched_back = call;
}

Fiber *get_curr_fiber()
{
    return curr_fiber;
}

void switch_to_worker_sched()
{
    assert(swapcontext(&curr_fiber->ctx, &thread_main_sched_ctx) == 0);
}

}
