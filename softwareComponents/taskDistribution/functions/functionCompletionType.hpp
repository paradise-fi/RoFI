#pragma once

enum FunctionCompletionType
{
    BlockingUntilReaction, // Task must wait for a response from the leader
    Blocking, // Task waits for a general signal and does not get unblocked by a response from the leader
    NonBlocking, // Task does not wait.
};