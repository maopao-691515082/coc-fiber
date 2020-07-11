#include <coc_fiber.h>
#include "internal.h"

namespace coc_fiber
{

static const size_t DEFAULT_STK_SIZE = 128 * 1024;

static std::mutex fiber_seq_lock;
static int64_t curr_fiber_seq = 0;

Fiber::Fiber(std::function<void ()> _run) : run(_run), _finished(false)
{
    std::lock_guard<std::mutex> fiber_seq_lock_guard(fiber_seq_lock);

    stk = new char[DEFAULT_STK_SIZE];

    ++ curr_fiber_seq;
    _seq = curr_fiber_seq;

    assert(getcontext(&ctx) == 0);
    ctx.uc_stack.ss_sp = stk;
    ctx.uc_stack.ss_size = DEFAULT_STK_SIZE;
    ctx.uc_link = nullptr;
    makecontext(&ctx, Fiber::start, 0);
}

Fiber::~Fiber()
{
    delete[] stk;
}

void Fiber::Start()
{
    Fiber *curr_fiber = get_curr_fiber();
    curr_fiber->run();
    curr_fiber->_finished = true;
    switch_to_worker_sched();
}

void create_fiber(std::function<void ()> run)
{
    reg_fiber(new Fiber(run));
}

}
