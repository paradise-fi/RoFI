#pragma once

enum DistributionMessageType {
    TaskRequest,
    TaskAssignment,
    TaskResult,
    TaskFailed,
    BlockingTaskRelease,
    DataStorageRequest,
    DataRemovalRequest,
    CustomMessage,
};