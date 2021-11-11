#pragma once


#define STM32CXX_USE_MEMORY_POOL
#define STM32CXX_MEMORY_BUCKETS \
    memory::Bucket< 1025, 20 >, \
    memory::Bucket< 128, 5 >, \
    memory::Bucket< 64, 10 >, \
    memory::Bucket< 32, 20 >

#define STM32CXX_DBG_ALLOCATOR memory::Pool
