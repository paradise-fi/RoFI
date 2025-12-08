#pragma once

enum class CompletionType
{
    Blocking, // Task waits for a general signal
    NonBlocking, // Task does not wait.
};