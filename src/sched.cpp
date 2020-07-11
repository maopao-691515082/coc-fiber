#include <coc_fiber.h>
#include "internal.h"

namespace coc_fiber
{

typedef std::map<int64_t, Fiber *> Fibers;

static std::mutex ready_fibers_lock;
static Fibers ready_fibers;

static std::mutex expire_queue_lock;
static std::map<int64_t, Fibers> expire_queue;

struct FdInfo
{
    std::mutex fd_info_lock;
    Fibers r_queue, w_queue;
};
static FdInfo fd_infos[1024 * 1024];

static int ep_fd = -1;
static int ev_fd = -1;

static void wake_up_fibers(const Fibers &fibers_to_wake_up)
{
    std::lock_guard<std::mutex> ready_fibers_lock_guard(ready_fibers_lock);

    for (auto fiber_iter = fibers_to_wake_up.begin(); fiber_iter != fibers_to_wake_up.end(); ++ fiber_iter)
    {
        int64_t fiber_seq = fiber_iter->first;
        Fiber *fiber = fiber_iter->second;

        ready_fibers[fiber_seq] = fiber;
    }
}

void clear_waiting_ev(Fiber *fiber)
{
    WaitingEvents &evs = fiber->waiting_evs;

    if (evs.expire_at >= 0)
    {
        std::lock_guard<std::mutex> expire_queue_lock_guard(expire_queue_lock);

        auto iter = expire_queue.find(evs.expire_at);
        if (iter != expire_waiting_fibers.end())
        {
            iter->second.erase(fiber->seq());
        }
        evs.expire_at = -1;
    }

#define CLEAR_FIBER_FROM_IO_WAITING_FIBERS(_r_or_w) do {                \
    int fd = evs.waiting_fd_##_r_or_w;                                  \
    if (fd >= 0) {                                                      \
        FdInfo &fd_info = fd_infos[fd];                                 \
        std::lock_guard<std::mutex> fd_info_lock_guard(fd_info_lock);   \
        fd_info._r_or_w##_queue.erase(fiber->seq());                    \
    }                                                                   \
    evs.waiting_fd_##_r_or_w = -1;                                      \
} while (false)

        CLEAR_FIBER_FROM_IO_WAITING_FIBERS(r);
        CLEAR_FIBER_FROM_IO_WAITING_FIBERS(w);

#undef CLEAR_FIBER_FROM_IO_WAITING_FIBERS
}

void init()
{
    ep_fd = epoll_create1(0);
    assert(ep_fd != -1);

    ev_fd = eventfd(0, EFD_NONBLOCK);
    assert(ev_fd != -1);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = ev_fd;
    assert(epoll_ctl(ep_fd, EPOLL_CTL_ADD, ev_fd, &ev) == 0);

    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);

    init_workers();
}

void reg_fiber(Fiber *fiber)
{
    std::lock_guard<std::mutex> ready_fibers_lock_guard(ready_fibers_lock);
    ready_fibers[fiber->seq()] = fiber;
}

void dispatch()
{
    for (;;)
    {
        {
            std::lock_guard<std::mutex> ready_fibers_lock_guard(ready_fibers_lock);
            if (ready_fibers.size() > 0)
            {
                std::list<Fiber *> fibers;
                for (auto iter = ready_fibers.begin(); iter != ready_fibers.end(); ++ iter)
                {
                    clear_waiting_ev(iter->second);
                    fibers.push_back(iter->second);
                }
                ready_fibers.clear();
                deliver_fiber_to_workers(fibers);
            }
        }

        int ep_timeout_ms = 100;
        {
            std::lock_guard<std::mutex> expire_queue_lock_guard(expire_queue_lock);

            int64_t now = now_ms();
            while (expire_queue.size() > 0)
            {
                auto iter = expire_queue.begin();
                int64_t expire_at = iter->first;
                if (expire_at > now)
                {
                    ep_timeout_ms = std::min(ep_timeout_ms, (int)(expire_at - now));
                    break;
                }
                Fibers fibers_to_wake_up(std::move(iter->second));
                expire_queue.erase(iter);
                wake_up_fibers(fibers_to_wake_up);
                ep_timeout_ms = 0;
            }
        }

        {
            static const int EPOLL_WAIT_EVENT_MAX_COUNT = 32;
            struct epoll_event evs[EPOLL_WAIT_EVENT_MAX_COUNT];
            int ev_count = epoll_wait(ep_fd, evs, EPOLL_WAIT_EVENT_MAX_COUNT, ep_timeout_ms);
            if (ev_count == -1)
            {
                assert(errno == EINTR);
                ev_count = 0;
            }

            if (ev_count > 0)
            {
                for (int i = 0; i < ev_count; ++ i)
                {
                    const struct epoll_event &ev = evs[i];
                    int fd = ev.data.fd;
                    if (fd == ev_fd)
                    {
                        for (;;)
                        {
                            uint64_t count;
                            int ret = read(ev_fd, &count, sizeof(count));
                            if (ret == -1)
                            {
                                assert(errno == EAGAIN);
                                break;
                            }
                        }
                        continue;
                    }
                    FdInfo &fd_info = fd_infos[fd];

                    {
                        std::lock_guard<std::mutex> fd_info_lock_guard(fd_info_lock);

#define WAKE_UP_BY_EVENT(_ev, _r_or_w) do {                             \
    if (ev.events & (EPOLL##_ev | EPOLLERR | EPOLLHUP)) {               \
        Fibers fibers_to_wake_up(std::move(fd_info._r_or_w##_queue));   \
        fd_info._r_or_w##_queue.clear();                                \
        wake_up_fibers(fibers_to_wake_up);                              \
    }                                                                   \
} while (false)

                        WAKE_UP_BY_EVENT(IN, r);
                        WAKE_UP_BY_EVENT(OUT, w);

#undef WAKE_UP_BY_EVENT

                    }
                }
            }
        }
    }
}

}
