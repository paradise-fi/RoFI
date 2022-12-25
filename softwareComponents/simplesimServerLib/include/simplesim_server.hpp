#pragma once

#include <cassert>
#include <filesystem>
#include <iostream>

#include <atoms/cmdline_utils.hpp>
#include <atoms/parsing.hpp>
#include <atoms/result.hpp>
#include <atoms/unreachable.hpp>
#include <dimcli/cli.h>

// RoFI headers
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>
#include <simplesim/packet_filters/py_filter.hpp>


namespace rofi::simplesim
{
using atoms::RofiWorldFormat;

class SimplesimServerOpts {
public:
    SimplesimServerOpts( const SimplesimServerOpts & ) = delete;
    SimplesimServerOpts & operator=( const SimplesimServerOpts & ) = delete;

    SimplesimServerOpts( Dim::Cli & cli )
    {
        cli.opt( &inputWorldFile, "<input_world_file>" )
                .defaultDesc( {} )
                .desc( "Source rofi world file ('-' for standard input)" );
        cli.opt( &rofiWorldFormat, "f format" )
                .valueDesc( "ROFI_WORLD_FORMAT" )
                .desc( "Format of the rofi world file" )
                .choice( RofiWorldFormat::Json, "json" )
                .choice( RofiWorldFormat::Old, "old" );

        cli.opt( &pyPacketFilterFile, "p python" )
                .valueDesc( "PYTHON_FILE" )
                .defaultDesc( {} )
                .desc( "Python packet filter file" );

        cli.opt( &verbose, "v verbose" ).desc( "Run simulator in verbose mode" );
    }

    auto readInputWorldFile() const -> atoms::Result< rofi::configuration::RofiWorld >
    {
        if ( verbose ) {
            std::cout << "Reading configuration from file (" << rofiWorldFormat
                      << " format, file: '" << inputWorldFile << "')\n";
        }
        return atoms::readInput( inputWorldFile, [ & ]( std::istream & istr ) {
            return atoms::parseRofiWorld( istr, rofiWorldFormat );
        } );
    }

    auto getPyPacketFilter() const -> std::optional< simplesim::packetf::PyFilter >
    {
        if ( !pyPacketFilterFile ) {
            return std::nullopt;
        }

        if ( verbose ) {
            std::cout << "Reading python packet filter from file (file: " << *pyPacketFilterFile
                      << ") \n";
        }
        auto packetFilterCode = atoms::readInputToString( *pyPacketFilterFile );
        return simplesim::packetf::PyFilter( packetFilterCode );
    }

    std::filesystem::path inputWorldFile = {};
    atoms::RofiWorldFormat rofiWorldFormat = {};
    std::optional< std::filesystem::path > pyPacketFilterFile = {};

    bool verbose = {};
};

} // namespace rofi::simplesim
