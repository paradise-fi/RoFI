#pragma once

#include <filesystem>
#include <fstream>
#include <istream>
#include <memory>
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
 * @brief Converts \p rofiWorld to `std::shared_ptr`
 * and validates the world.
 * Returns an error if \p rofiWorld is not valid.
 * @note Expected use with `atoms::Result::and_then`.
 * @note Uses `std::shared_ptr` because moving
 * `rofi::configuration::RofiWorld` unprepares it.
 * @param rofiWorld input rofi world
 * @returns the validated \p rofiWorld inside a `std::shared_ptr`
 */
inline auto toSharedAndValidate( rofi::configuration::RofiWorld rofiWorld )
        -> atoms::Result< std::shared_ptr< rofi::configuration::RofiWorld > >
{
    using rofi::configuration::RofiWorld;
    auto result = std::make_shared< RofiWorld >( std::move( rofiWorld ) );
    assert( result );
    if ( auto valid = result->validate(); !valid ) {
        return valid.assume_error_result();
    } else {
        return atoms::result_value( std::move( result ) );
    }
}

/**
 * @brief Validates rofi worlds in \p rofiWorldSeq .
 * Returns an error if any world in \p rofiWorldSeq is not valid.
 * @note Expected use with `atoms::Result::and_then`.
 * @param rofiWorldSeq input rofi world sequence
 * @returns validated \p rofiWorldSeq
 */
inline auto validateSequence( std::vector< rofi::configuration::RofiWorld > rofiWorldSeq )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    for ( auto & rofiWorld : rofiWorldSeq ) {
        if ( auto valid = rofiWorld.validate(); !valid ) {
            return valid.assume_error_result();
        }
    }
    return atoms::result_value( std::move( rofiWorldSeq ) );
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


enum class RofiWorldFormat {
    Old = -1,
    Json,
};

inline auto operator<<( std::ostream & ostr, RofiWorldFormat configFormat ) -> std::ostream &
{
    switch ( configFormat ) {
        case RofiWorldFormat::Old:
            return ostr << "old";
        case RofiWorldFormat::Json:
            return ostr << "json";
    }
    assert( false );
    return ostr << static_cast< int >( configFormat );
}

/**
 * @brief Parses rofi world from given \p istr .
 * Parses the input according to \p worldFormat .
 * If the world is in old format, fixates the world.
 * Returns an error if the format is incorrect.
 * @param istr input stream containing the rofi world
 * @param worldFormat format of input world
 * @returns the parsed rofi world
 */
inline auto parseRofiWorld( std::istream & istr, RofiWorldFormat worldFormat )
        -> atoms::Result< rofi::configuration::RofiWorld >
{
    switch ( worldFormat ) {
        case RofiWorldFormat::Old:
            return atoms::parseOldCfgFormat( istr, true );
        case RofiWorldFormat::Json:
            return atoms::parseRofiWorldJson( istr );
    }
    ROFI_UNREACHABLE( "Unknown configuration format" );
}

} // namespace atoms
