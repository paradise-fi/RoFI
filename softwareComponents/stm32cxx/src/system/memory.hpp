#pragma once

#include <stm32cxx.config.hpp>

#include <cassert>
#include <cstdint>
#include <memory>

namespace memory::detail {

template < int SIZE, int COUNT, typename Self >
class BlockPool_;

constexpr int reduceCount( int size, int /* count */ ) {
    if ( size <= 32 )
        return 4;
    return 1;
}

constexpr int reduceSize( int size, int /* count */ ) {
    if ( size <= 8 )
        return 0;
    return size / 2;
}

template < int SIZE, int COUNT >
class BlockPool {
protected:
    using Next = BlockPool_< SIZE, COUNT, BlockPool< SIZE, COUNT > >;
    struct Deleter {
        void operator()( uint8_t mem[] ) { BlockPool::free( mem ); }
    };

public:
    using Block = std::unique_ptr< uint8_t[], Deleter >;

    static Block allocate( int size ) {
        return Next::allocate( size );
    }
private:
    static void free( uint8_t *mem ) {
        Next::free( mem );
    }
};

template < int SIZE, int COUNT, typename Self >
class BlockPool_ {
    static const int nextSize = reduceSize( SIZE, COUNT );
    static const int nextCount = reduceCount( SIZE, COUNT );
    using Next = BlockPool_< nextSize, nextCount, Self >;
    using Block = typename Self::Block;
public:
    static Block allocate( int size ) {
        if ( size > SIZE || size == 0 )
            return Block();
        if ( size <= nextSize ) {
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
        uint8_t mem[ SIZE ] __attribute__(( aligned( 4 ) ));
    };

    static std::array< Mem, COUNT >& _pool() {
        static std::array< Mem, COUNT > pool;
        return pool;
    }
};

template < int COUNT, typename Self >
class BlockPool_< 0, COUNT, Self > {
    using Block = typename Self::Block;
public:
    static Block allocate( int /* size */ ) {
        return Block( nullptr );
    }

    static void free( uint8_t * /* mem */ ) {
        assert( false && "Out of pool pointer" );
    }
};

} // namespace memory::detail

namespace memory {

#ifdef STM32CXX_USE_MEMORY_POOL
    #if !defined(STM32CXX_MEMORY_BLOCK_SIZE) || !defined(STM32CXX_MEMORY_BLOCK_COUNT)
        #error To use memory pool STM32CXX_MEMORY_BLOCK_SIZE and STM32CXX_MEMORY_BLOCK_COUNT have to defined
    #endif

    using Pool = detail::BlockPool< STM32CXX_MEMORY_BLOCK_SIZE, STM32CXX_MEMORY_BLOCK_COUNT >;
#endif

} // namespace memory