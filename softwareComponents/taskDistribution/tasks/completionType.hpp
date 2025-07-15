#pragma once

enum CompletionType
{
    Blocking, // Task waits for a general signal
    NonBlocking, // Task does not wait.
};