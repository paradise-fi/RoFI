#pragma once


#define STM32CXX_USE_MEMORY_POOL
#define STM32CXX_MEMORY_BUCKETS \
    memory::Bucket< 2048, 5 >, \
    memory::Bucket< 1024, 10 >, \
    memory::Bucket< 256, 5 >, \
    memory::Bucket< 128, 5 >, \
    memory::Bucket< 64, 255 >, \
    memory::Bucket< 32, 20 >

#define STM32CXX_DBG_ALLOCATOR memory::Pool

#define STM32CXX_IRQ_HIGH_PRIORITY 3
