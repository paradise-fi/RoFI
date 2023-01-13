#pragma once

#include <cassert>
#include <filesystem>
#include <iostream>

#include <atoms/cmdline_utils.hpp>
#include <atoms/result.hpp>
#include <atoms/unreachable.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing_lite.hpp>
#include <simplesim/packet_filters/py_filter.hpp>


namespace rofi::simplesim
{
class SimplesimServerOpts {
public:
    SimplesimServerOpts( const SimplesimServerOpts & ) = delete;
    SimplesimServerOpts & operator=( const SimplesimServerOpts & ) = delete;

    SimplesimServerOpts( Dim::Cli & cli )
    {
        cli.opt( &inputWorldFile, "<input_world_file>" )
                .defaultDesc( {} )
                .desc( "Source rofi world file ('-' for standard input)" );

        cli.opt( &pyPacketFilterFile, "p python" )
                .valueDesc( "python_file" )
                .defaultDesc( {} )
                .desc( "Python packet filter file" );

        cli.opt( &verbose, "v verbose" ).desc( "Run simulator in verbose mode" );
    }

    auto readInputWorldFile() const -> atoms::Result< rofi::configuration::RofiWorld >
    {
        if ( verbose ) {
            std::cout << "Reading configuration from file '" << inputWorldFile << "'\n";
        }
        return atoms::readInput( inputWorldFile, rofi::parsing::parseJson )
                .and_then( rofi::parsing::getRofiWorldFromJson );
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
    std::optional< std::filesystem::path > pyPacketFilterFile = {};

    bool verbose = {};
};

} // namespace rofi::simplesim
