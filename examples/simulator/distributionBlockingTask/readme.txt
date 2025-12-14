This example only serves to show the difference between blocking function tasks and barrier function tasks. Note that both blocking and barrier functions
must be marked as Blocking in FunctionCompletionType.

Blocking Function Tasks do not prevent other tasks from being queued infront of them once they are in the queue, you may set a higher priority to attempt
to queue a task before a blocking function tasks that has already been queued.

Barrier Function Tasks prevent other tasks from being queued infront of them once they are in the queue, no matter the priority.

The intended outcome is that all non-barrier function tasks return their values in a monotone rising sequence. Check resultLogs/2_log.txt for the correct output.