#include <lwip/pbuf.h>
#include <lwip/netif.h>
#include <lwip/ip.h>

#include <type_traits>
#include <sstream>

namespace rofi::hal {

/**
 * \brief Wrapper over LwIP ip6_addr_t.
 * 
 * Provides a few useful methods and C++ operators.
 */
struct Ip6Addr : ip6_addr_t {
	Ip6Addr( const char* str ) {
		ip6addr_aton( str, this );
    }
    Ip6Addr( ip6_addr addr ) : ip6_addr( addr ) {}
	Ip6Addr( ip6_addr&& o ) {
		std::swap( addr, o.addr );
	}

	Ip6Addr( uint8_t mask ) {
		mask_to_address( mask, this );
	}

    bool operator==( const Ip6Addr& o ) const {
		return addr[0] == o.addr[0] && addr[1] == o.addr[1] && addr[2] == o.addr[2]
					&& addr[3] == o.addr[3];
	}

	bool operator!=( const Ip6Addr& o ) const {
		return !( o == *this );
	}

	struct Ip6Addr operator&( const Ip6Addr& mask ) const {
		Ip6Addr res( *this );
		for ( int i = 0; i < 4; i++ )
			res.addr[ i ] = addr[ i ] & mask.addr[ i ];
		return res;
	}

    static int size() { return 16; }
};

inline std::ostream& operator<<( std::ostream& o, const Ip6Addr& a ) {
	o << ip6addr_ntoa( &a );
	return o;
}

typedef struct netif netif_t;

/**
 * \brief Wrapper over LwIP netif.
 * 
 * Provides few useful methods.
 */
struct Netif : netif_t {
	std::string getName() const {
		std::ostringstream s;
		s << name[ 0 ] << name[ 1 ] << static_cast< int >( num );
		return s.str();
	}

	bool isStub() const {
		return stub;
	}

	bool setStub( bool b ) {
		return stub = b;
	}

	bool isActive() const {
		return active;
	}

	bool setActive( bool b ) {
		return active = b;
	}
private:
	bool stub   = false;
	bool active = false;
};

/**
 * \brief RAII wrapper over LwIP pbuff.
 *
 * Makes the PBuf non-copyable, but movable.
 */
class PBuf {
public:
    static PBuf allocate( int size ) { return PBuf( size ); }
    static PBuf reference( pbuf* b ) { return PBuf( b, true ); }
    static PBuf own( pbuf* b ) { return PBuf( b, false ); }
    static PBuf empty() { return PBuf( nullptr, false ); }

    PBuf( const PBuf& o ): _buff( o._buff ) { pbuf_ref( _buff ); };
    PBuf& operator=( PBuf o ) { swap( o );  return *this; }
    PBuf( PBuf&& o ): _buff( o._buff ) { o._buff = nullptr; }
    PBuf& operator=( PBuf&& o ) { swap( o ); return *this; }

    ~PBuf() { if ( _buff ) pbuf_free( _buff ); }

    void swap( PBuf& o ) {
        using std::swap;
        swap( _buff, o._buff );
    }

    /**
     * \brief Release owenership of the packet
     *
     * This function should be called when the packet is passed to a C interface
     * which manages its lifetime.
     */
    pbuf* release() {
        auto b = _buff;
        _buff = nullptr;
        return b;
    }

    pbuf* get() { return _buff; }

    operator bool() const { return _buff; }
    bool simple() const { return _buff->tot_len == _buff->len; }
    uint8_t* payload() { return reinterpret_cast< uint8_t * >( _buff->payload ); }
    const uint8_t* payload() const {
        return reinterpret_cast< const uint8_t * >( _buff->payload );
    }
    int size() const { return _buff->tot_len; }

    uint8_t& operator[]( int idx ) { return get( *this, idx ); }
    const uint8_t& operator[]( int idx ) const { return get( *this, idx ); }

    struct Chunk {
        uint8_t* mem(){ return reinterpret_cast< uint8_t * >( _buf->payload ); };
        int size() { return _buf->len; }
        const pbuf* _buf;
    };

    class ChunkIt {
    public:
        using value_type = Chunk;
        using reference = Chunk&;
        using pointer = Chunk*;
        typedef int difference_type;
        typedef std::forward_iterator_tag iterator_category;

        ChunkIt( pbuf* buf = nullptr ) : _c{ buf } { }

        ChunkIt operator++( int ) {
            ChunkIt i = *this;
            ++(*this);
            return i;
        }
        ChunkIt& operator++() {
            // See loop end condition: https://www.nongnu.org/lwip/2_0_x/group__pbuf.html
            if ( _c._buf->tot_len == _c._buf->len )
                _c._buf = nullptr;
            else
                _c._buf = _c._buf->next;
            return *this;
        }
        reference operator*() { return _c; }
        pointer operator->() { return &_c; }
        bool operator==( const ChunkIt& o ) const { return _c._buf == o._c._buf; }
        bool operator!=( const ChunkIt& o ) const { return !( *this == o ); }
    private:
        Chunk _c;
    };

    ChunkIt chunksBegin() { return ChunkIt( _buff ); }
    ChunkIt chunksEnd() { return ChunkIt(); }

    std::string asString() {
        std::string s;
        for ( auto it = chunksBegin(); it != chunksEnd(); ++it ) {
            std::copy_n( it->mem(), it->size(), std::back_inserter( s ) );
        }
        return s;
    }
private:
    PBuf( pbuf* buff, bool addReference ) : _buff( buff ) {
        if ( addReference )
            pbuf_ref( _buff );
    }
    PBuf( int size ) : _buff( pbuf_alloc( PBUF_RAW, size, PBUF_POOL ) ) {
        if ( !_buff )
            throw std::bad_alloc();
    }

    template< typename Self,
              typename R = typename std::conditional<
                std::is_const< Self >::value, const uint8_t , uint8_t >::type >
    static R& get( Self& s, int idx ) {
        auto* chunk = s._buff;
        while( idx > chunk->len ) {
            idx -= chunk->len;
            chunk = chunk->next;
        }
        return static_cast< R* >( chunk->payload )[ idx ];
    }

    pbuf* _buff;
};

} // namespace rofi::hal