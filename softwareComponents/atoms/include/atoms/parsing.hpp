#pragma once

#include <filesystem>
#include <fstream>
#include <istream>
#include <stdexcept>
#include <vector>

#include <atoms/result.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>
#include <nlohmann/json.hpp>


namespace atoms
{

/**
 * @brief Fixates given \p world in space using a `RigidJoint` between
 * (0, 0, 0) and first body of first module.
 * Assumes the world modules is not empty.
 * @param world rofi world to be fixated in space
 */
inline void fixateRofiWorld( rofi::configuration::RofiWorld & world )
{
    using namespace rofi::configuration;
    auto modules = world.modules();
    assert( !modules.empty() );
    const auto & firstModule = modules.begin()->module;
    assert( firstModule );
    assert( !firstModule->components().empty() );
    auto component = !firstModule->bodies().empty() ? firstModule->bodies().front()
                                                    : firstModule->components().front();
    connect< RigidJoint >( component, {}, matrices::identity );
}


/**
 * @brief Parses rofi world from given \p istr .
 * Assumes that the input is in json format.
 * Returns an error if the format is incorrect.
 * @param istr input stream containing the rofi world
 * @returns the parsed rofi world
 */
inline auto parseRofiWorldJson( std::istream & istr )
        -> atoms::Result< rofi::configuration::RofiWorld >
{
    namespace serialization = rofi::configuration::serialization;
    try {
        auto json = nlohmann::json::parse( istr );
        return atoms::result_value( serialization::fromJSON( json ) );
    } catch ( const std::exception & e ) {
        return atoms::result_error< std::string >( e.what() );
    }
}

/**
 * @brief Parses rofi world from given \p istr .
 * Assumes that the input is in old (Viki) format.
 * Returns an error if the format is incorrect.
 * @param istr input stream containing the rofi world
 * @param fixate whether to fixate the world after parsing
 * @returns the parsed rofi world
 */
inline auto parseOldCfgFormat( std::istream & istr, bool fixate )
        -> atoms::Result< rofi::configuration::RofiWorld >
{
    using namespace rofi::configuration;
    try {
        auto rofiWorld = readOldConfigurationFormat( istr );
        if ( fixate ) {
            fixateRofiWorld( rofiWorld );
        }
        return atoms::result_value( std::move( rofiWorld ) );
    } catch ( const std::exception & e ) {
        return atoms::result_error< std::string >( e.what() );
    }
}


/**
 * @brief Parses rofi world sequence from given \p istr .
 * Assumes that the input is in json format
 * and that it contains an array of rofi worlds.
 * Returns an error if the format is incorrect.
 * @param istr input stream containing the rofi world sequence
 * @returns the parsed rofi world sequence
 */
inline auto parseRofiWorldSeqJson( std::istream & istr )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    using namespace rofi::configuration;
    try {
        auto json = nlohmann::json::parse( istr );
        if ( !json.is_array() ) {
            return atoms::result_error< std::string >( "Expected an array of rofi worlds" );
        }

        auto result = std::vector< RofiWorld >();
        result.reserve( json.size() );
        for ( auto & worldJson : json ) {
            result.push_back( serialization::fromJSON( worldJson ) );
        }
        return atoms::result_value( std::move( result ) );
    } catch ( const std::exception & e ) {
        return atoms::result_error< std::string >( e.what() );
    }
}


/**
 * @brief Parses and validates rofi world from given \p istr .
 * Assumes that the input is in json format.
 * Returns an error if the format is incorrect or is not valid.
 * @param istr input stream containing the rofi world
 * @returns the parsed and validated rofi world
 */
inline auto parseAndValidateRofiWorldJson( std::istream & istr )
        -> atoms::Result< std::unique_ptr< rofi::configuration::RofiWorld > >
{
    using rofi::configuration::RofiWorld;
    return parseRofiWorldJson( istr ).and_then(
            []( RofiWorld rofiWorld ) -> atoms::Result< std::unique_ptr< RofiWorld > > {
                auto result = std::make_unique< RofiWorld >( std::move( rofiWorld ) );
                assert( result );
                if ( auto valid = result->validate(); !valid ) {
                    return valid.assume_error_result();
                } else {
                    return atoms::result_value( std::move( result ) );
                }
            } );
}

/**
 * @brief Parses, fixates and validates rofi world from given \p istr .
 * Assumes that the input is in old (Viki) format.
 * Returns an error if the format is incorrect or is not valid.
 * @param istr input stream containing the rofi world
 * @returns the parsed and validated rofi world
 */
inline auto parseAndValidateOldCfgFormat( std::istream & istr )
        -> atoms::Result< std::unique_ptr< rofi::configuration::RofiWorld > >
{
    using rofi::configuration::RofiWorld;
    return parseOldCfgFormat( istr, true )
            .and_then( []( RofiWorld rofiWorld ) -> atoms::Result< std::unique_ptr< RofiWorld > > {
                auto result = std::make_unique< RofiWorld >( std::move( rofiWorld ) );
                assert( result );
                if ( auto valid = result->validate(); !valid ) {
                    return valid.assume_error_result();
                } else {
                    return atoms::result_value( std::move( result ) );
                }
            } );
}

/**
 * @brief Parses rofi world sequence from given \p istr .
 * Assumes that the input is in json format
 * and that it contains an array of rofi worlds.
 * Returns an error if the format is incorrect.
 * @param istr input stream containing the rofi world sequence
 * @returns the parsed rofi world sequence
 */
inline auto parseAndValidateRofiWorldSeqJson( std::istream & istr )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    using rofi::configuration::RofiWorld;
    return parseRofiWorldSeqJson( istr ).and_then(
            []( auto rofiWorldSeq ) -> atoms::Result< std::vector< RofiWorld > > {
                for ( RofiWorld & rofiWorld : rofiWorldSeq ) {
                    if ( auto valid = rofiWorld.validate(); !valid ) {
                        return valid.assume_error_result();
                    }
                }
                return atoms::result_value( std::move( rofiWorldSeq ) );
            } );
}


/**
 * @brief Parses rofi world from given \p inputFile .
 * If file has .json extension, assumes the JSON format of rofi world;
 * otherwise assumes the world is in the old (viki) format.
 * If the world is in old format, fixates the world.
 * Returns an error if the format is incorrect.
 * @param inputFile path to file containing the rofi world
 * @returns the parsed rofi world
 */
inline auto parseRofiWorld( const std::filesystem::path & inputFile )
        -> atoms::Result< rofi::configuration::RofiWorld >
{
    auto istr = std::ifstream( inputFile );
    if ( !istr.is_open() ) {
        return atoms::result_error( "Cannot open file '" + inputFile.generic_string() + "'" );
    }

    if ( inputFile.extension() == ".json" ) {
        return parseRofiWorldJson( istr );
    } else {
        return parseOldCfgFormat( istr, true );
    }
}

} // namespace atoms
