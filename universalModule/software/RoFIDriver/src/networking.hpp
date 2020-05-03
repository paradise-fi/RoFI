#include <lwip/pbuf.h>
#include <type_traits>

namespace rofi::hal {

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
