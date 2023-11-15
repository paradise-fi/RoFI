#include <string>
#include <functional>

namespace rofi::util {
class LineReader {
public:
    template < typename Callback >
    void bind( Callback onNewLine ) {
        _onNewLine = onNewLine;
    }

    void push( const char* data, int length ) {
        for ( int i = 0; i != length; i++ )
            push( data[ i ] );
    }

    void push( const std::string& s ) {
        for ( char c : s )
            push( c );
    }

    void push( char c ) {
        if ( c == '\n' ) {
            _onNewLine( std::move( _line ) );
            _line = "";
        }
        else {
            _line.push_back( c );
        }
    }

    void finish() {
        if ( !_line.empty() )
            _onNewLine( std::move( _line ) );
        _line = "";
    }
private:
    std::string _line;
    std::function< void( std::string&& )> _onNewLine;
};

} // namespace rofi::util
