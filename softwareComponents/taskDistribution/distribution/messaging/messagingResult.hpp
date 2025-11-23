#pragma once
#include <string>
#include <vector>

struct MessagingResult
{
    bool success;
    std::string statusMessage;
    std::vector< uint8_t > rawData;
};