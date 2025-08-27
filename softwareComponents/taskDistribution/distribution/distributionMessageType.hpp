#pragma once

enum DistributionMessageType {
    TaskRequest,
    TaskAssignment,
    TaskResult,
    TaskFailed,
    MalformedMessage,
    FollowerBusy,
    DataStorageRequest,
    DataStorageSuccess,
    BlockingTaskRelease
};