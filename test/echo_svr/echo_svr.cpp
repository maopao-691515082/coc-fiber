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

static void dispatch_client(coc_fiber::Conn conn)
{
    for (;;)
    {
        char buf[1024];
        ssize_t ret = conn.read(buf, sizeof(buf));
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
        if (conn.write(buf, (size_t)ret) != 0)
        {
            log_err("write to client error");
            return;
        }
    }
}

static void start_svr()
{
    auto listener = coc_fiber::ListenTCP(9999);
    if (!listener.valid())
    {
        exit_with_err("listen failed");
    }
    for (;;)
    {
        auto conn = listener.accept();
        if (!conn.valid())
        {
            log_err("accept failed");
        }
        coc_fiber::create_fiber([conn] () {
            dispatch_client(conn);
            conn.close();
        });
    }
}

int main(int argc, char *argv[])
{
    coc_fiber::init();
    coc_fiber::create_fiber(start_svr);
    coc_fiber::dispatch();
}
