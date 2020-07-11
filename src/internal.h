#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <list>
#include <map>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <iostream>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ucontext.h>
#include <sys/eventfd.h>

namespace coc_fiber
{

#define assert(_cond) do {                                                                          \
    if (!(_cond)) {                                                                                 \
        std::cerr << "assert failed: [" __FILE__ "][" << __LINE__ << "] " << #_cond << std::endl;   \
        exit(1);                                                                                    \
    }                                                                                               \
} while (false)

int64_t now_ms();

struct WaitingEvents
{
    int64_t expire_at = -1;
    int waiting_fd_r = -1, waiting_fd_w = -1;
};

class Fiber
{
    std::function<void ()> run;
    bool _finished = false;
    char *stk;
    int64_t _seq;

    static void start();

public:

    Fiber(std::function<void ()> run);
    ~Fiber();

    bool finished()
    {
        return _finished;
    }

    int64_t seq()
    {
        return _seq;
    }

    WaitingEvents waiting_evs;
    ucontext_t ctx;
};

void init_workers();
void deliver_fiber_to_workers(const std::list<Fiber *> &fibers);
void set_call_after_sched_back(std::function<void ()> call);
Fiber *get_curr_fiber();
void switch_to_worker_sched();

void reg_fiber(Fiber *fiber);
void reg_fd(int fd);
void unreg_fd(int fd);
void notify_main_sched_ev_fd();
std::mutex *get_fd_info_lock(int fd);
std::mutex *wait_expire(int64_t expire_at);
std::mutex *wait_readable(int fd, int64_t expire_at);
std::mutex *wait_writable(int fd, int64_t expire_at);

}
