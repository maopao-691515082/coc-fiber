#include <stdlib.h>

#include <iostream>

#include <coc_fiber.h>

static void log(const std::string &msg)
{
    std::cout << msg << std::endl;
}

static void log_err(const std::string &err)
{
    std::cerr << err << std::endl;
}

static void exit_with_err(const std::string &err)
{
    log_err(err);
    exit(1);
}

static void dispatch_client(int conn_fd)
{
    log("new client");
    for (;;)
    {
        char buf[1024];
        ssize_t ret = coc_fiber::read_data(conn_fd, buf, sizeof(buf));
        if (ret < 0)
        {
            log_err("read from client error");
            return;
        }
        if (ret == 0)
        {
            log("client ends");
            return;
        }
        if (coc_fiber::write_data(conn_fd, buf, (size_t)ret) != 0)
        {
            log_err("write to client error");
            return;
        }
    }
}

static void start_svr()
{
    int listen_fd = coc_fiber::listen_tcp(9736);
    if (listen_fd < 0)
    {
        exit_with_err("listen failed");
    }
    for (;;)
    {
        int conn_fd = coc_fiber::accept_fd(listen_fd);
        if (conn_fd < 0)
        {
            log_err("accept failed");
            coc_fiber::sleep_ms(1);
            continue;
        }
        coc_fiber::create_fiber([conn_fd] () {
            dispatch_client(conn_fd);
            coc_fiber::close_fd(conn_fd);
        });
    }
}

int main(int argc, char *argv[])
{
    coc_fiber::init();
    coc_fiber::create_fiber(start_svr);
    coc_fiber::dispatch();
}
