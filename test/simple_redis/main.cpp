#include "internal.h"

namespace simple_redis
{

void dispatch_client(int conn_fd)
{
    RedisConn redis_conn(conn_fd);

    for (;;)
    {
        std::vector<std::string> args;
        if (!redis_conn.recv_args(&args))
        {
            log("recv_args error");
            return;
        }
        if (args.size() == 0)
        {
            log("client ends");
            return;
        }

        std::string rsp;
        proc_req_args(args, &rsp);

        if (!redis_conn.send_rsp(rsp))
        {
            log("send_rsp err");
            return;
        }
    }
}

void main()
{
    int listen_fd = coc_fiber::listen_tcp(9010);
    assert(listen_fd >= 0);
    for (;;)
    {
        int conn_fd = coc_fiber::accept_fd(listen_fd);
        if (conn_fd < 0)
        {
            log("accept failed");
            coc_fiber::sleep_ms(1);
            continue;
        }
        coc_fiber::create_fiber([conn_fd] () {
            dispatch_client(conn_fd);
        });
    }

}

}

int main(int argc, char *argv[])
{
    coc_fiber::init();
    coc_fiber::create_fiber(simple_redis::main);
    coc_fiber::dispatch();
}
