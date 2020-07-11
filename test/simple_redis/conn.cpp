#include "internal.h"

namespace simple_redis
{

RedisConn::RedisConn(int fd) : conn_fd(fd)
{
}

RedisConn::~RedisConn()
{
    coc_fiber::close_fd(conn_fd);
}

bool RedisConn::fill_recv_buf()
{
    if (recv_buf_len > 0)
    {
        return true;
    }

    recv_buf_start = 0;
    ssize_t ret = coc_fiber::read_data(conn_fd, recv_buf, sizeof(recv_buf));
    if (ret < 0)
    {
        return false;
    }
    recv_buf_len = (size_t)ret;
    return true;
}

bool RedisConn::read_fixed_char(char ch)
{
    if (fill_recv_buf() && recv_buf_len > 0 && recv_buf[recv_buf_start] == ch)
    {
        ++ recv_buf_start;
        -- recv_buf_len;
        return true;
    }
    return false;
}

bool RedisConn::read_int(int64_t *n)
{
    std::string s;
    for (;;)
    {
        if (!fill_recv_buf() || recv_buf_len == 0)
        {
            return false;
        }
        bool ok = false;
        while (recv_buf_len > 0 && s.size() < 32)
        {
            char ch = recv_buf[recv_buf_start];
            ++ recv_buf_start;
            -- recv_buf_len;
            if (ch == '\r')
            {
                ok = true;
                break;
            }
            s.append(&ch, 1);
        }
        if (ok)
        {
            break;
        }
        if (s.size() >= 32)
        {
            return false;
        }
    }
    if (read_fixed_char('\n'))
    {
        return false;
    }

    if (!str_to_int64(s, n))
    {
        return false;
    }

    return true;
}

bool RedisConn::read_str(std::string *s)
{
    s->clear();

    if (!read_fixed_char('$'))
    {
        return false;
    }

    int64_t n;
    if (!read_int(&n))
    {
        return false;
    }
    if (n < 0)
    {
        return false;
    }
    size_t s_len = (size_t)n;

    while (s->size() < s_len)
    {
        if (!fill_recv_buf() || recv_buf_len == 0)
        {
            return false;
        }
        size_t copy_len = std::min(recv_buf_len, s_len - s->size());
        s->append(recv_buf + recv_buf_start, copy_len);
        recv_buf_start += copy_len;
        recv_buf_len -= copy_len;
    }

    return true;
}

bool RedisConn::recv_args(std::vector<std::string> *args)
{
    args->clear();

    if (!fill_recv_buf())
    {
        return false;
    }
    if (recv_buf_len == 0)
    {
        return true;
    }

    if (!read_fixed_char('*'))
    {
        return false;
    }

}

bool RedisConn::send_rsp(const std::string &rsp)
{
    return coc_fiber::write_data(conn_fd, rsp.data(), rsp.size(), 1000) == 0;
}

}
