#pragma once

#include <stdlib.h>

#include <functional>

namespace coc_fiber
{

void init();
void create_fiber(std::function<void ()> run);
void dispatch();

int listen_tcp(uint16_t port);
int accept_fd(int listen_fd, int64_t timeout_ms = -1);
ssize_t read_data(int fd, char *buf, size_t sz, int64_t timeout_ms = -1);
int write_data(int fd, const char *buf, size_t sz, int64_t timeout_ms = -1);
void close_fd(int fd);

void sleep_ms(int64_t ms);

}
