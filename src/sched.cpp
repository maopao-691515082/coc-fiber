#include <coc_fiber.h>
#include "internal.h"

namespace coc_fiber
{

typedef std::map<int64_t, Fiber *> Fibers;

static std::mutex ready_fibers_lock;
static Fibers ready_fibers;

static int ep_fd = -1;

void init()
{
    ep_fd = epoll_create1(0);
    assert(ep_fd != -1);

    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);

    init_workers();
}

void reg_fiber(Fiber *fiber)
{
    std::lock_guard<std::mutex> ready_fibers_lock_guard(ready_fibers_lock);
    ready_fibers[fiber->seq()] = fiber;
}

}
