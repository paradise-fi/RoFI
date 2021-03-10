#pragma once

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace lorris
{
class Packet
{
public:
    Packet() : _data( 4, 0 )
    {
        _data.at( 0 ) = 0x55;
    }

private:
    void setSize( char size )
    {
        assert( size >= 0 );
        assert( 4 + _data.at( 1 ) == static_cast< int >( _data.size() ) );
        _data.resize( 4 + size );
        _data.at( 1 ) = size;
    }

public:
    const char & size() const
    {
        return _data.at( 1 );
    }

    char & command()
    {
        return _data.at( 2 );
    }

    const char & command() const
    {
        return _data.at( 2 );
    }

    char & devId()
    {
        return _data.at( 3 );
    }

    const char & devId() const
    {
        return _data.at( 3 );
    }

    template < typename T >
    void push( T t )
    {
        int offset = _data.size();
        setSize( size() + sizeof( T ) );
        assert( _data.size() >= offset + sizeof( T ) );
        reinterpret_cast< T & >( _data.at( offset ) ) = t;
    }

    void push( const std::string & s )
    {
        for ( char c : s )
        {
            push< char >( c );
        }
        push< char >( 0 );
    }

    template < typename T >
    T get( int offset ) const
    {
        return reinterpret_cast< T & >( _data.at( offset ) );
    }

    void send() const;

private:
    std::vector< char > _data;
    friend class Connector;
};

class Connector
{
public:
    Connector( int port ) : _port( port )
    {
        using namespace std::string_literals;

        _socketFd = socket( AF_INET, SOCK_DGRAM, 0 );
        if ( _socketFd < 0 )
        {
            throw std::runtime_error( "Cannot open socket: "s + strerror( errno ) );
        }
        sockaddr_in servaddr = {};
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons( port );
        int bound = bind( _socketFd,
                          reinterpret_cast< const struct sockaddr * >( &servaddr ),
                          sizeof( servaddr ) );
        if ( bound < 0 )
        {
            throw std::runtime_error( "Cannot bind socket: "s + strerror( errno ) );
        }
    }

    void send( const Packet & pkt )
    {
        using namespace std::string_literals;

        sockaddr_in cliaddr = {};
        cliaddr.sin_family = AF_INET;
        cliaddr.sin_port = htons( _port + 1 );
        inet_aton( "localhost", &cliaddr.sin_addr );
        ssize_t sent = sendto( _socketFd,
                               pkt._data.data(),
                               pkt._data.size(),
                               0,
                               reinterpret_cast< struct sockaddr * >( &cliaddr ),
                               sizeof( cliaddr ) );
        if ( sent < 0 )
        {
            throw std::runtime_error( "Cannot send: "s + strerror( errno ) );
        }
    }

    static Connector & instance()
    {
        static Connector conn( 8888 );
        return conn;
    }

private:
    int _socketFd;
    int _port;
};

inline void Packet::send() const
{
    Connector::instance().send( *this );
}

} // namespace lorris
