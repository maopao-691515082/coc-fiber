#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

#include <coc_fiber.h>

namespace simple_redis
{

#define assert(_cond) do {                                                          \
    if (!(_cond)) {                                                                 \
        fprintf(stderr, "assert failed [%s][%d] %s\n", __FILE__, __LINE__, #_cond); \
        exit(1);                                                                    \
    }                                                                               \
} while (false)

void log(const char *fmt, ...);
bool str_to_int64(const std::string &s, int64_t *v);

class RedisConn
{
    int conn_fd = -1;
    char recv_buf[4096];
    size_t recv_buf_start = 0, recv_buf_len = 0;

    bool fill_recv_buf();
    bool read_fixed_char(char ch);
    bool read_int(int64_t *n);
    bool read_str(std::string *s);

public:

    RedisConn(int fd);
    ~RedisConn();

    bool recv_args(std::vector<std::string> *args);
    bool send_rsp(const std::string &rsp);
};

void proc_req_args(const std::vector<std::string> &args, std::string *rsp);

void do_set(const std::string &key, const std::string &value);
bool do_get(const std::string &key, std::string *value);

}
