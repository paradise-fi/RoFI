#pragma once

#include <concepts>
#include <filesystem>
#include <iostream>
#include <memory>

#include <dimcli/cli.h>

#include "configuration/rofiworld.hpp"
#include "configuration/serialization.hpp"


inline auto readJsonCfgFromStream( std::istream & istr )
        -> std::shared_ptr< rofi::configuration::RofiWorld >
{
    auto rofiWorld = rofi::configuration::serialization::fromJSON( nlohmann::json::parse( istr ) );

    auto rofiWorldPtr = std::make_shared< rofi::configuration::RofiWorld >( std::move( rofiWorld ) );
    rofiWorldPtr->prepare();
    if ( auto [ ok, str ] = rofiWorldPtr->isValid( rofi::configuration::SimpleCollision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return rofiWorldPtr;
}


class UserConfig {
public:
    static auto readInputCfg( std::invocable< std::istream & > auto readCfgCallback )
    {
        if ( inputCfgFileName == "-" ) {
            return readCfgCallback( std::cin );
        } else {
            auto inputFile = std::ifstream( inputCfgFileName );
            if ( !inputFile.is_open() ) {
                throw std::runtime_error( "Cannot open input config file '"
                                          + inputCfgFileName.generic_string() + "'" );
            }
            return readCfgCallback( inputFile );
        }
    }

    static void writeOutputCfg( std::invocable< std::ostream & > auto writeCfgCallback )
    {
        if ( outputCfgFileName == "-" ) {
            writeCfgCallback( std::cout );
        } else {
            auto outputFile = std::ofstream( outputCfgFileName );
            if ( !outputFile.is_open() ) {
                throw std::runtime_error( "Cannot open output config file '"
                                          + outputCfgFileName.generic_string() + "'" );
            }
            writeCfgCallback( outputFile );
        }
    }

private:
    friend int main( int argc, char * argv[] );

    static std::filesystem::path inputCfgFileName;
    static std::filesystem::path outputCfgFileName;
};
