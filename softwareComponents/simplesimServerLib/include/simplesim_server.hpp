#pragma once

#include <cassert>
#include <filesystem>
#include <ostream>

#include <dimcli/cli.h>

// RoFI headers
#include <configuration/rofibot.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>
#include <simplesim/packet_filters/py_filter.hpp>


namespace rofi::simplesim
{
enum class ConfigurationFormat
{
    Old = -1,
    Json,
};

inline auto operator<<( std::ostream & ostr, ConfigurationFormat configFormat ) -> std::ostream &
{
    switch ( configFormat ) {
        case ConfigurationFormat::Old:
            return ostr << "old";
        case ConfigurationFormat::Json:
            return ostr << "json";
    }
    assert( false );
    return ostr << static_cast< int >( configFormat );
}

inline auto cfgFilePathCliOpt( Dim::Cli & cli ) -> Dim::Cli::Opt< std::filesystem::path > &
{
    return cli.opt< std::filesystem::path >( "<input_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Configuration file" );
}

inline auto cfgFormatCliOpt( Dim::Cli & cli ) -> Dim::Cli::Opt< ConfigurationFormat > &
{
    return cli.opt< ConfigurationFormat >( "f format" )
            .valueDesc( "CONFIGURATION_FORMAT" )
            .defaultDesc( {} )
            .desc( "Format of the configuration file" )
            .choice( ConfigurationFormat::Json, "json" )
            .choice( ConfigurationFormat::Old, "old" );
}

inline auto pyPacketFilterFilePathCliOpt( Dim::Cli & cli )
        -> Dim::Cli::Opt< std::filesystem::path > &
{
    return cli.opt< std::filesystem::path >( "p python" )
            .valueDesc( "PYTHON_FILE" )
            .defaultDesc( {} )
            .desc( "Python packet filter file" );
}

auto readAndPrepareConfigurationFromFile( const std::filesystem::path & cfgFilePath,
                                          ConfigurationFormat configFormat )
        -> std::shared_ptr< const rofi::configuration::Rofibot >;

auto readPyFilterFromFile( const std::filesystem::path & packetFilterFilePath )
        -> packetf::PyFilter;

} // namespace rofi::simplesim
