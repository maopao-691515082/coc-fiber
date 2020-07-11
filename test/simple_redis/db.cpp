#include "internal.h"

namespace simple_redis
{

struct DBSlot
{
    std::mutex lock;
    std::map<std::string, std::string> kvs;
};

static const size_t DB_SLOT_COUNT = 1000003;

static DBSlot db_slots[DB_SLOT_COUNT];
static std::atomic<int64_t> kv_count;

static size_t str_hash(const std::string &s)
{
    size_t sz = s.size();
    size_t h = sz;
    for (size_t i = 0; i < sz; ++ i)
    {
        h = (h + (unsigned char)s[i]) * 29722590580933441ULL;
    }
    return h;
}

static DBSlot &get_db_slot(const std::string &key)
{
    return db_slots[str_hash(key) % DB_SLOT_COUNT];
}

void do_set(const std::string &key, const std::string &value)
{
    DBSlot &slot = get_db_slot(key);
    std::lock_guard<std::mutex> db_slot_lock_guard(slot.lock);

    auto result = slot.kvs.insert(make_pair(key, value));
    if (result.second)
    {
        ++ kv_count;
    }
    else
    {
        result.first->second = value;
    }
}

bool do_get(const std::string &key, std::string *value)
{
    DBSlot &slot = get_db_slot(key);
    std::lock_guard<std::mutex> db_slot_lock_guard(slot.lock);

    auto iter = slot.kvs.find(key);
    if (iter == slot.kvs.end())
    {
        return false;
    }

    *value = iter->second;
    return true;
}

}
