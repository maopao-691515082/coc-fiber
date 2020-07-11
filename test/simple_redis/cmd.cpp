#include "internal.h"

namespace simple_redis
{

void proc_req_args(const std::vector<std::string> &args, std::string *rsp)
{
    const std::string &cmd = args.at(0);
    if (cmd == "SET")
    {
        if (args.size() != 3)
        {
            rsp->assign("-ERR invalid arg count\r\n");
            return;
        }
        const std::string &key = args.at(1);
        const std::string &value = args.at(2);

        do_set(key, value);

        rsp->assign("+OK\r\n");
    }
    else if (cmd == "GET")
    {
        if (args.size() != 2)
        {
            rsp->assign("-ERR invalid arg count\r\n");
            return;
        }
        const std::string &key = args.at(1);
        std::string value;

        if (do_get(key, &value))
        {
            char value_len[32];
            sprintf(value_len, "$%zu\r\n", value.size());
            rsp->assign(value_len);
            rsp->append(value);
            rsp->append("\r\n");
        }
        else
        {
            rsp->assign("$-1\r\n");
        }
    }
    else
    {
        rsp->assign("-ERR invalid cmd\r\n");
    }
}

}
