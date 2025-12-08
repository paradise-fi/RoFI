#pragma once

enum class FunctionDistributionType
{
    Unicast, // Function tasks are only sent to a specific module address
    Broadcast, // Function tasks are distributed to all modules at the same time
};