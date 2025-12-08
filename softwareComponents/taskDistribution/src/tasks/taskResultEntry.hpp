#pragma once

#include "lwip++.hpp"
#include "task.hpp"

struct TaskResultEntry
{
    std::unique_ptr< TaskBase > task;
    Ip6Addr origin;
};