#include <catch.hpp>
#include <system/memory.hpp>

using MyPool = memory::detail::BlockPool< 1024, 10 >;

TEST_CASE( "Pool: capacity" ) {
    std::vector< typename MyPool::Block > blocks;
    int blockId = 0;
    auto newValidBlob = [&]( int size ) {
        auto b = MyPool::allocate( size );
        REQUIRE( b );
        for ( const auto& other : blocks ) {
            REQUIRE( b.get() != other.get() );
        }
        b[ 0 ] = ++blockId;
        blocks.push_back( std::move( b ) );
    };

    SECTION( "Combination of memory sizes in order" ) {
        // 10 large blocks can be allocated
        for ( int i = 0; i != 10; i++ ) {
            INFO( "Allocating blob " << ( i + 1 ) );
            newValidBlob( 900 );
        }
        // No more large blocks can be allocated
        auto bLarge = MyPool::allocate( 900 );
        REQUIRE( !bLarge );
        // Smaller block can be allocated
        newValidBlob( 512 );
        // After releasing a block a new one can be allocated
        int victim = 7;
        uint8_t *mem = blocks[ victim ].get();
        blocks.erase( blocks.begin() + victim );
        bLarge = MyPool::allocate( 900 );
        REQUIRE( bLarge );
        REQUIRE( bLarge.get() == mem );
    }

    SECTION ( "Combinatin of memory sizes out of order" ) {
        // Allocate some small blocks
        newValidBlob( 10 );
        newValidBlob( 30 );
        newValidBlob( 64 );
        newValidBlob( 78 );
        newValidBlob( 512 );
        newValidBlob( 40 );
        // 10 large blocks can be allocated
        for ( int i = 0; i != 10; i++ ) {
            INFO( "Allocating blob " << ( i + 1 ) );
            newValidBlob( 900 );
        }
        // No more large blocks can be allocated
        auto bLarge = MyPool::allocate( 900 );
        REQUIRE( !bLarge );
        // Free small block
        blocks.erase( blocks.begin() + 2 );
        // No more large blocks can be allocated
        bLarge = MyPool::allocate( 900 );
        REQUIRE( !bLarge );
        // Small black can be allocated
        newValidBlob( 10 );
        // Free large block
        blocks.erase( blocks.begin() + 9 );
        newValidBlob( 1024 );
    }
}

TEST_CASE( "Pool: edge cases" ) {
    auto b1 = MyPool::allocate( 2000 );
    REQUIRE( !b1 );

    auto b2 = MyPool::allocate( 0 );
    REQUIRE( !b2 );
}