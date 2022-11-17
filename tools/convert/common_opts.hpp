#pragma once

#include <concepts>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include <dimcli/cli.h>

#include "atoms/result.hpp"
#include "configuration/rofiworld.hpp"
#include "configuration/serialization.hpp"


// Returns shared pointer because moving un-prepares the RofiWorld
inline auto readJsonCfgFromStream( std::istream & istr )
        -> atoms::Result< std::shared_ptr< rofi::configuration::RofiWorld > >
{
    using namespace std::string_literals;
    using namespace rofi::configuration;

    std::shared_ptr< RofiWorld > rofiWorldPtr;
    try {
        auto rofiWorld = serialization::fromJSON( nlohmann::json::parse( istr ) );

        rofiWorldPtr = std::make_shared< RofiWorld >( std::move( rofiWorld ) );
    } catch ( const nlohmann::json::exception & e ) {
        return atoms::result_error( "Error while parsing RofiWorld ("s + e.what() + ")" );
    }

    assert( rofiWorldPtr );
    if ( auto ok = rofiWorldPtr->validate( rofi::configuration::SimpleCollision() ); !ok ) {
        return std::move( ok ).assume_error_result();
    }
    return atoms::result_value( std::move( rofiWorldPtr ) );
}


auto readInput( const std::filesystem::path & inputFilePath,
                std::invocable< std::istream & > auto readCallback )
{
    if ( inputFilePath == "-" ) {
        return readCallback( std::cin );
    } else {
        auto inputFile = std::ifstream( inputFilePath );
        if ( !inputFile.is_open() ) {
            throw std::runtime_error( "Cannot open input config file '"
                                      + inputFilePath.generic_string() + "'" );
        }
        return readCallback( inputFile );
    }
}

auto writeOutput( const std::filesystem::path & outputFilePath,
                  std::invocable< std::ostream & > auto writeCallback )
{
    if ( outputFilePath == "-" ) {
        writeCallback( std::cout );
    } else {
        auto outputFile = std::ofstream( outputFilePath );
        if ( !outputFile.is_open() ) {
            throw std::runtime_error( "Cannot open output config file '"
                                      + outputFilePath.generic_string() + "'" );
        }
        writeCallback( outputFile );
    }
}
