#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <optional>
#include <ranges>

#include <iostream>

namespace rofi::net {

class Logger {
public:
    enum class Level { Info
                     , Debug
                     , Warn
                     , Error
    };

    using Where = std::string;
    using Log = std::tuple< Level, std::optional< Where >, std::string >;
    using LogFunction = std::function< void ( Logger::Level, const std::string, const std::string ) >;

    explicit Logger( std::ostream& o ) : _out( { o } ) {};
    explicit Logger() = default;

    ~Logger() {
        if ( _out )
            operator<<( *_out );
    }

    void log( Level l, const std::string& msg ) {
        _logs.push_back( { l, std::nullopt, msg } );
    }

    void log( Level l, const std::string& where, const std::string& msg ) {
        _logs.push_back( { l, { where }, msg } );
    }

    void logInfo( const std::string& msg ) {
        log( Level::Info, msg );
    }

    void logDebug( const std::string& msg ) {
        log( Level::Debug, msg );
    }

    void logWarn( const std::string& msg ) {
        log( Level::Warn, msg );
    }

    void logError( const std::string& msg ) {
        log( Level::Error, msg );
    }

    template< typename T >
    static std::string toString( const T& t ) {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }

    auto logs() const {
        return std::views::all( _logs );
    }

    auto logsFrom( const Where& w ) const {
        return logs() | std::views::filter( [ w ]( const Log& l ) {
            return std::get< 1 >( l ) && std::get< 1 >( l ).value() == w;
        } );
    }

    auto logs( Level severity ) const {
        return std::views::filter( _logs, [ severity ]( const Log& l ) {
            return std::get< 0 >( l ) == severity;
        } );
    }

    static void printLog( std::ostream& o, const Log& l ) {
        auto& [ level, where, msg ] = l;
        o << "LEVEL " << static_cast< int >( level ) << ": ";
        if ( where )
            o << "[" << *where << "]: ";
        o << msg << "\n";
    }

    std::ostream& operator<<( std::ostream& o ) const {
        for ( auto l : _logs ) {
            printLog( o, l );
        }
        return o;
    }

private:
    std::vector< Log > _logs;
    std::optional< std::reference_wrapper< std::ostream > > _out;
};

} // namespace rofi::net
