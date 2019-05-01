#include <catch.hpp>
#include <system/ringBuffer.hpp>
#include <cstddef>

TEST_CASE( "ringBuffer: basic" ) {
    RingBuffer< uint32_t > buffer( memory::Pool::allocate( 32 ), 32 );
    INFO( "Initial check" );
    CHECK( buffer.capacity() == 7 );
    CHECK( buffer.size() == 0 );
    CHECK( buffer.available() == 7 );
    CHECK( buffer.empty() );
    CHECK( !buffer.full() );

    for ( int i = 0; i != 20; i++ ) {
        INFO( "Iteration " << i );

        buffer.push_back( 1 );
        INFO( "Insert 1" );
        CHECK( buffer.capacity() == 7 );
        CHECK( buffer.size() == 1 );
        CHECK( buffer.available() == 6 );
        CHECK( !buffer.empty() );
        CHECK( !buffer.full() );
        CHECK( buffer[ 0 ] == 1 );

        buffer.push_back( 2 );
        INFO( "Insert 2" );
        CHECK( buffer.capacity() == 7 );
        CHECK( buffer.size() == 2 );
        CHECK( buffer.available() == 5 );
        CHECK( !buffer.empty() );
        CHECK( !buffer.full() );
        CHECK( buffer[ 0 ] == 1 );
        CHECK( buffer[ 1 ] == 2 );

        buffer.push_back( 3 );
        INFO( "Insert 3" );
        CHECK( buffer.capacity() == 7 );
        CHECK( buffer.size() == 3 );
        CHECK( buffer.available() == 4 );
        CHECK( !buffer.empty() );
        CHECK( !buffer.full() );
        CHECK( buffer[ 0 ] == 1 );
        CHECK( buffer[ 1 ] == 2 );
        CHECK( buffer[ 2 ] == 3 );

        CHECK( buffer.pop_front() == 1 );
        INFO( "Pop 1" );
        CHECK( buffer.capacity() == 7 );
        CHECK( buffer.size() == 2 );
        CHECK( buffer.available() == 5 );
        CHECK( !buffer.empty() );
        CHECK( !buffer.full() );
        CHECK( buffer[ 0 ] == 2 );
        CHECK( buffer[ 1 ] == 3 );

        CHECK( buffer.pop_front() == 2 );
        INFO( "Pop 2" );
        CHECK( buffer.capacity() == 7 );
        CHECK( buffer.size() == 1 );
        CHECK( buffer.available() == 6 );
        CHECK( !buffer.empty() );
        CHECK( !buffer.full() );
        CHECK( buffer[ 0 ] == 3 );

        CHECK( buffer.pop_front() == 3 );
        INFO( "Pop 3" );
        CHECK( buffer.capacity() == 7 );
        CHECK( buffer.size() == 0 );
        CHECK( buffer.available() == 7 );
        CHECK( buffer.empty() );
        CHECK( !buffer.full() );
    }
}

TEST_CASE( "ringBuffer: insert space & advance" ) {
    RingBuffer< uint32_t > buffer( memory::Pool::allocate( 32 ), 32 );
    auto [ b, space ] = buffer.insertPosition();
    CHECK( space == 7 );

    buffer.push_back( 1 );
    std::tie( b, space ) = buffer.insertPosition();
    CHECK( space == 6 );
    CHECK( buffer[ 0 ] ==  1 );

    b[ 0 ] = 2;
    b[ 1 ] = 3;
    b[ 2 ] = 4;
    buffer.advance( 3 );
    CHECK( buffer.size() == 4 );
    for ( int i = 0; i != 4; i++ )
        CHECK( buffer[ i ] == i + 1 );

    buffer.push_back( 5 );
    buffer.push_back( 6 );
    buffer.push_back( 7 );

    buffer.pop_front();
    buffer.pop_front();
    buffer.pop_front();

    std::tie( b, space ) = buffer.insertPosition();
    CHECK( space == 1 );

    buffer.push_back( 8 );
    std::tie( b, space ) = buffer.insertPosition();
    CHECK( space == 2 );
}