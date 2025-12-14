#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring>
#include "messagingResult.hpp"

class BlockingMessageDataService
{
    std::condition_variable _blockingMessageCV;
    std::mutex _lock;
    std::vector< uint8_t > _blockingDataBuffer;
    bool _dataReady = false;

public:
    void completeBlockingMessage( uint8_t* message, size_t messageSize );

    void clearFlag();

    MessagingResult awaitBlockingMessage( int timeout );
};