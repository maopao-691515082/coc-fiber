#pragma once

#include <stdlib.h>

#include <functional>

namespace coc_fiber
{

void init();
void create_fiber(std::function<void ()> run);
void dispatch();

}
