#pragma once

#include <memory>

#include "task.hpp"
#include "../functions/enums/functionCompletionType.hpp"

struct TaskEntry
{
    TaskEntry( std::unique_ptr< TaskBase >&& task, FunctionCompletionType completionType )
    : task( std::move( task ) ), completionType( completionType ) {}

    std::unique_ptr< TaskBase > task;
    FunctionCompletionType completionType;
};