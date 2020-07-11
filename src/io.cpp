#include <coc_fiber.h>
#include "internal.h"

namespace coc_fiber
{

int listen_tcp(uint16_t port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd != -1);

    int reuseaddr_on = 1;
    assert(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on)) != -1);

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);

    assert(bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) != -1);

    assert(listen(listen_fd, 1024) != -1);

    reg_fd(listen_fd);

    return listen_fd;
}

int accept_fd(int listen_fd, int64_t timeout_ms)
{
    assert(listen_fd >= 0);

    int64_t expire_at = timeout_ms < 0 ? -1 : now_ms() + timeout_ms;
    std::mutex *fd_info_lock = get_fd_info_lock(listen_fd);
    for (;;)
    {
        fd_info_lock->lock();
        int fd = accept(listen_fd, nullptr, nullptr);
        if (fd >= 0)
        {
            reg_fd(fd);
            fd_info_lock->unlock();
            return fd;
        }
        assert(fd == -1 && errno != 0);
        if (errno != EAGAIN && errno != EINTR)
        {
            fd_info_lock->unlock();
            return -1;
        }
        if (expire_at >= 0 && expire_at <= now_ms())
        {
            fd_info_lock->unlock();
            return -2;
        }
        if (errno == EAGAIN)
        {
            std::mutex *expire_queue_lock = wait_readable(listen_fd, expire_at);
            set_call_after_sched_back([fd_info_lock, expire_queue_lock] () {
                if (expire_queue_lock != nullptr)
                {
                    expire_queue_lock->unlock();
                }
                fd_info_lock->unlock();
            });
            switch_to_worker_sched();
        }
        else
        {
            fd_info_lock->unlock();
        }
    }
}

ssize_t read_data(int fd, char *buf, size_t sz, int64_t timeout_ms)
{
    assert(fd >= 0 && buf != nullptr && sz > 0);

    int64_t expire_at = timeout_ms < 0 ? -1 : now_ms() + timeout_ms;
    std::mutex *fd_info_lock = get_fd_info_lock(fd);
    for (;;)
    {
        fd_info_lock->lock();
        ssize_t ret = read(fd, buf, sz);
        if (ret >= 0)
        {
            fd_info_lock->unlock();
            return ret;
        }
        assert(ret == -1 && errno != 0);
        if (errno != EAGAIN && errno != EINTR)
        {
            fd_info_lock->unlock();
            return -1;
        }
        if (expire_at >= 0 && expire_at <= now_ms())
        {
            fd_info_lock->unlock();
            return -2;
        }
        if (errno == EAGAIN)
        {
            std::mutex *expire_queue_lock = wait_readable(fd, expire_at);
            set_call_after_sched_back([fd_info_lock, expire_queue_lock] () {
                if (expire_queue_lock != nullptr)
                {
                    expire_queue_lock->unlock();
                }
                fd_info_lock->unlock();
            });
            switch_to_worker_sched();
        }
        else
        {
            fd_info_lock->unlock();
        }
    }
}

int write_data(int fd, const char *buf, size_t sz, int64_t timeout_ms)
{
    assert(fd >= 0 && buf != nullptr);

    int64_t expire_at = timeout_ms < 0 ? -1 : now_ms() + timeout_ms;
    std::mutex *fd_info_lock = get_fd_info_lock(fd);
    while (sz > 0)
    {
        fd_info_lock->lock();
        ssize_t ret = write(fd, buf, sz);
        if (ret >= 0)
        {
            assert((size_t)ret <= sz);
            buf += ret;
            sz -= (size_t)ret;
            fd_info_lock->unlock();
            continue;
        }
        assert(ret == -1 && errno != 0);
        if (errno != EAGAIN && errno != EINTR)
        {
            fd_info_lock->unlock();
            return -1;
        }
        if (expire_at >= 0 && expire_at <= now_ms())
        {
            fd_info_lock->unlock();
            return -2;
        }
        if (errno == EAGAIN)
        {
            std::mutex *expire_queue_lock = wait_writable(fd, expire_at);
            set_call_after_sched_back([fd_info_lock, expire_queue_lock] () {
                if (expire_queue_lock != nullptr)
                {
                    expire_queue_lock->unlock();
                }
                fd_info_lock->unlock();
            });
            switch_to_worker_sched();
        }
        else
        {
            fd_info_lock->unlock();
        }
    }
}

void close_fd(int fd)
{
    unreg_fd(fd);
    close(fd);
}

}
