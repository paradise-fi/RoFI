#pragma once

enum class FunctionCompletionType
{
    Blocking, // Task waits for a general signal and does not get unblocked by a response from the leader
    NonBlocking, // Task does not wait.
};