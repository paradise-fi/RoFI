#include <memory>

#include "task.hpp"
#include "completionType.hpp"

struct TaskEntry
{
    TaskEntry( std::unique_ptr< TaskBase >&& task, CompletionType completionType )
    : task( std::move( task ) ), completionType( completionType ) {}

    std::unique_ptr< TaskBase > task;
    CompletionType completionType;
};