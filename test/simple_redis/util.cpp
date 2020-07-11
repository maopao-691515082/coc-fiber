#include "internal.h"

namespace simple_redis
{

void log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");
}

bool str_to_int64(const std::string &s, int64_t *v)
{
    const char *p = s.c_str();
    const char *end_ptr;
    errno = 0;
    long long llv = strtoll(p, (char **)&end_ptr, 10);
    if (*p != '\0' && *end_ptr == '\0' && end_ptr == p + s.size() && errno == 0)
    {
        *v = llv;
        return true;
    }
    else
    {
        return false;
    }
}

}
