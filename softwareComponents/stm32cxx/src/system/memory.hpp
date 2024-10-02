#pragma once

#include <stm32cxx.config.hpp>

#include <system/assert.hpp>
#include <cstdint>
#include <memory>

namespace memory::detail {

struct BucketSentinel {};

template < typename Self, typename Bucket, typename... Buckets >
class BlockPool_ {
    using Next = BlockPool_< Self, Buckets... >;
    using Block = typename Self::Block;
public:
    static constexpr int Size = Bucket::Size;
    static constexpr int Count = Bucket::Count;

    static Block allocate( int size ) {
        if ( size > Size || size == 0 )
            return Block();
        if ( size <= Next::Size ) {
            Block b = Next::allocate( size );
            if ( b )
                return b;
        }
        for ( auto& x : _pool() ) {
            if ( !x.available )
                continue;
            x.available = false;
            return Block( x.mem );
        }
        return Block();
    }

    static void free( uint8_t *mem ) {
        for ( auto& x : _pool() ) {
            if ( x.mem != mem )
                continue;
            x.available = true;
            mem = nullptr;
            return;
        }
        Next::free( mem );
    }
private:
    struct Mem {
        bool available = true;
        uint8_t mem[ Size ] __attribute__(( aligned( 4 ) ));
    };

    static std::array< Mem, Count >& _pool() {
        static std::array< Mem, Count > pool;
        return pool;
    }
};

template < typename Self >
class BlockPool_< Self, BucketSentinel > {
    // The Bucket is always empty, this is ensured by BlockPool
    using Block = typename Self::Block;
public:
    static constexpr int Size = 0;
    static constexpr int Count = 0;

    static Block allocate( int /* size */ ) {
        return Block();
    }

    static void free( uint8_t * /* mem */ ) {
        assert( false && "Out of pool pointer" );
    }
};

template < typename... Buckets >
class BlockPool {
protected:
    using Impl = BlockPool_< BlockPool, Buckets..., BucketSentinel >;
    struct Deleter {
        void operator()( uint8_t mem[] ) { BlockPool::free( mem ); }
    };

public:
    using Block = std::unique_ptr< uint8_t[], Deleter >;

    static Block allocate( int size ) {
        return Impl::allocate( size );
    }
private:
    static void free( uint8_t *mem ) {
        Impl::free( mem );
    }
};

} // namespace memory::detail

namespace memory {

template < int SIZE, int COUNT >
struct Bucket {
    static constexpr int Size = SIZE;
    static constexpr int Count = COUNT;
};

#ifdef STM32CXX_USE_MEMORY_POOL
    #if !defined(STM32CXX_MEMORY_BUCKETS)
        #error To use memory pool STM32CXX_MEMORY_BUCKETS has to be defined
    #endif

    using Pool = detail::BlockPool< STM32CXX_MEMORY_BUCKETS >;
#endif

} // namespace memory
