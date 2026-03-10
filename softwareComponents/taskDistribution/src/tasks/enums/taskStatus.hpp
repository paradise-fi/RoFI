#pragma once

enum class TaskStatus {
    Pending, // Default status
    Complete, // Returned with a success
    Failed, // Returned with a failure
    RepeatLocally, // Reschedule at the executor
    RepeatDistributed, // Reschedule at the central authority / leader
};
