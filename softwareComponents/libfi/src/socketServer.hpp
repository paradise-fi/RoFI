#include <sys/socket.h>
#include <netinet/in.h>

#include <functional>
#include <cstring>
#include <thread>

namespace rofi::net {

class SocketClient {
public:
    SocketClient( const SocketClient& ) = delete;
    SocketClient& operator=( const SocketClient& ) = delete;
    SocketClient( SocketClient&& other ) {
        swap( other );
    }
    SocketClient& operator=( SocketClient&& other ) {
        swap( other );
        return *this;
    }

    ~SocketClient() {
        close();
    }

    /**
     * Create new socket by taking ownership of existing file descriptor
     */
    static SocketClient take( int fd ) {
        return SocketClient( fd );
    }

    void swap( SocketClient& other ) {
        using std::swap;
        swap( _fd, other._fd );
    }

    void close() {
        if ( _fd != -1 ) {
            ::close( _fd );
            _fd = -1;
        }
    }

    void write( const char *message ) {
        int err = ::write( _fd, message, strlen( message ) );
        validatePosixError( err, "Cannot write: " );
    }

    int available() {
        int avail = 0;
        int err = ioctl( _fd, FIONREAD, &avail );
        validatePosixError( err, "Cannot check available bytes: " );
        return avail;
    }

    std::string readNoBlock( int maxLength = 128 ) {
        return read( maxLength, false );
    }

    std::string read( int maxLength = 128, bool blocking = true ) {
        int flags = 0;
        if ( !blocking )
            flags |= MSG_DONTWAIT;
        std::string s( maxLength, ' ' );
        int read = ::recv( _fd, s.data(), maxLength, flags );
        if ( read < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) )
            return {};
        validatePosixError( read, "Cannot read: " );
        s.resize( read );
        return s;
    }

private:
    SocketClient( int fd = -1 ): _fd( fd ) {}

    void validatePosixError( int errCode, const char* context ) {
        if ( errCode < 0 ) {
            int err = errno;
            std::string errNumMsg = "(" + std::to_string( err ) + ")";
            throw std::runtime_error( context + ( strerror( err ) + errNumMsg ));
        }
    }

    int _fd;
};

/**
 * Simple socket server. Each client is automatically accepted and served in a
 * separate thread.
 */
class SocketServer {
public:
    ~SocketServer() {
        close();
    }

    void bind( int port, int connectionLimit = 5 ) {
        close();

        _fd = socket( AF_INET, SOCK_STREAM, 0 );
        if ( _fd < 0 )
            throw std::runtime_error( strerror( errno ) );
        sockaddr_in sockAddr;
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_addr.s_addr = INADDR_ANY;
        sockAddr.sin_port = htons( port );
        int err = ::bind( _fd,
            reinterpret_cast< sockaddr *>( &sockAddr), sizeof( sockAddr ) );
        if ( err < 0 )
            throw std::runtime_error( strerror( errno ) );

        err = listen( _fd, connectionLimit );
        if ( err < 0 )
            throw std::runtime_error( strerror( errno ) );

        std::thread acceptThread( [this, fd = _fd]{ acceptRoutine( fd ); } );
        // We can safely detach as the thread will be released once we close
        // the socket.
        acceptThread.detach();
    }

    template < typename F >
    void setClientRoutine( F routine ) {
        _onClient = routine;
    }


    void close() {
        if ( _fd != -1 ) {
            ::close( _fd );
            _fd = -1;
        }
    }
private:
    /**
     * Note that we take fd as an argument to capture its value. In this way we
     * avoid possible race-condition with close.
     */
    void acceptRoutine( int fd ) {
        std::cout << "Starting accept thread!\n";
        while ( true ) {
            try {
                sockaddr_in sockAddr;
                socklen_t addrLen = sizeof( sockaddr );
                int connection = accept( fd,
                    reinterpret_cast< sockaddr *>( &sockAddr), &addrLen );
                std::cout << "Accept passed!\n";
                if ( connection < 0 ) {
                    int err = errno;
                    if ( err == ECONNABORTED )
                        return;
                    std::cout << strerror( err ) << "\n";
                    continue;
                }
                std::thread clientThread( [this, connection, sockAddr]{
                     _onClient( SocketClient::take( connection ), sockAddr );
                });
                clientThread.detach();
            } catch( std::runtime_error& e ) {
                std::cout << e.what() << "\n";
            }
        }
    }

    int _fd = -1;
    std::function< void( SocketClient, sockaddr_in )> _onClient;
};

} // namespace rofi::net
