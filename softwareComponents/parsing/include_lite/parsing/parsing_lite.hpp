#pragma once

#include <istream>
#include <memory>
#include <stdexcept>
#include <vector>

#include <atoms/result.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>
#include <nlohmann/json.hpp>


namespace rofi::parsing
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
    const auto & firstModule = modules.front();
    assert( !firstModule.components().empty() );
    auto component = !firstModule.bodies().empty() ? firstModule.bodies().front()
                                                   : firstModule.components().front();
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
    }
    return atoms::result_value( std::move( result ) );
}

/**
 * @brief Validates rofi worlds in \p rofiWorldSeq .
 * Returns an error if any world in \p rofiWorldSeq is not valid.
 * @param rofiWorldSeq input rofi world sequence
 */
inline auto validateRofiWorldSeq( std::span< rofi::configuration::RofiWorld > rofiWorldSeq )
        -> atoms::Result< std::monostate >
{
    for ( size_t i = 0; i < rofiWorldSeq.size(); i++ ) {
        if ( auto valid = rofiWorldSeq[ i ].validate(); !valid ) {
            return atoms::result_error( "Rofi world " + std::to_string( i )
                                        + " is not valid: " + valid.assume_error() );
        }
    }
    return atoms::result_value( std::monostate() );
}

/**
 * @brief Validates rofi worlds in \p rofiWorldSeq .
 * Returns an error if any world in \p rofiWorldSeq is not valid.
 * @note Expected use with `atoms::Result::and_then`.
 * @param rofiWorldSeq input rofi world sequence
 * @returns validated \p rofiWorldSeq
 */
inline auto validatedRofiWorldSeq( std::vector< rofi::configuration::RofiWorld > rofiWorldSeq )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    if ( auto valid = validateRofiWorldSeq( rofiWorldSeq ); !valid ) {
        return valid.assume_error_result();
    }
    return atoms::result_value( std::move( rofiWorldSeq ) );
}

/**
 * @brief Converts \p rofiWorldSeq to a sequence of new worlds.
 * Uses \p convertCallback for conversion to \p WorldT .
 * Returns an error if any call to \p convertCallback fails
 * including information about the position of the failed world.
 *
 * @note Behaves as transform and collect on `atoms::Result`
 * and also includes info about which conversion failed.
 * @param rofiWorldSeq input rofi world sequence
 * @param convertCallback callback for converting rofi world to new world
 * @returns converted sequence of new worlds
 */
template <
        typename Callback,
        typename WorldT =
                std::invoke_result_t< Callback, const rofi::configuration::RofiWorld >::value_type >
auto convertFromRofiWorldSeq( std::span< const rofi::configuration::RofiWorld > rofiWorldSeq,
                              Callback convertCallback ) -> atoms::Result< std::vector< WorldT > >
{
    static_assert( atoms::is_result_v<
                   std::invoke_result_t< Callback, const rofi::configuration::RofiWorld & > > );

    auto voxelWorldSeq = std::vector< WorldT >();
    voxelWorldSeq.reserve( rofiWorldSeq.size() );
    for ( size_t i = 0; i < rofiWorldSeq.size(); i++ ) {
        auto voxelWorld = convertCallback( rofiWorldSeq[ i ] );
        if ( !voxelWorld ) {
            return atoms::result_error( "Error converting rofi world " + std::to_string( i ) + ": "
                                        + voxelWorld.assume_error() );
        }
        voxelWorldSeq.push_back( std::move( *voxelWorld ) );
    }
    return atoms::result_value( std::move( voxelWorldSeq ) );
}


/**
 * @brief Converts \p worldSeq to a rofi world sequence.
 * Uses \p convertCallback for conversion to rofi world.
 * Returns an error if any call to \p convertCallback fails
 * including information about the position of the failed world.
 *
 * @note Behaves as transform and collect on `atoms::Result`
 * and also includes info about which conversion failed.
 * @param worldSeq input world sequence
 * @param convertCallback callback for converting world to rofi world
 * @returns converted rofi world sequence
 */
template < typename WorldT, typename Callback >
auto convertToRofiWorldSeq( std::span< const WorldT > worldSeq, Callback convertCallback )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    static_assert( atoms::is_result_v< std::invoke_result_t< Callback, const WorldT > > );
    static_assert(
            std::is_same_v< typename std::invoke_result_t< Callback, const WorldT >::value_type,
                            rofi::configuration::RofiWorld > );

    auto rofiWorldSeq = std::vector< rofi::configuration::RofiWorld >();
    rofiWorldSeq.reserve( worldSeq.size() );
    for ( size_t i = 0; i < worldSeq.size(); i++ ) {
        auto rofiWorld = convertCallback( worldSeq[ i ] );
        if ( !rofiWorld ) {
            return atoms::result_error( "Error converting world " + std::to_string( i )
                                        + " to rofi world: " + rofiWorld.assume_error() );
        }
        rofiWorldSeq.push_back( std::move( *rofiWorld ) );
    }
    return atoms::result_value( std::move( rofiWorldSeq ) );
}


/**
 * @brief Calls `nlohmann::json::parse`,
 * but returns `atoms::Result` instead of throwing.
 * Returns an error if parsing throws an exception.
 * @param istr input stream
 * @returns parsed json
 */
inline auto parseJson( std::istream & istr ) -> atoms::Result< nlohmann::json >
{
    using namespace std::string_literals;
    try {
        return atoms::result_value( nlohmann::json::parse( istr ) );
    } catch ( const nlohmann::json::exception & e ) {
        return atoms::result_error( "Error while parsing json: "s + e.what() );
    }
}

/**
 * @brief Calls `nlohmann::json::get`,
 * but returns `atoms::Result` instead of throwing.
 * Returns an error if converting throws an exception.
 * @tparam T value type to be returned
 * @param json input json
 * @returns converted value
 */
template < typename T >
auto getFromJson( const nlohmann::json & json ) -> atoms::Result< T >
{
    using namespace std::string_literals;
    try {
        return atoms::result_value( json.get< T >() );
    } catch ( const std::exception & e ) {
        return atoms::result_error( "Error while converting json to value: "s + e.what() );
    }
}


/**
 * @brief Calls `rofi::configuration::serialization::fromJSON`,
 * but returns `atoms::Result` instead of throwing.
 * Returns an error if converting throws an exception.
 * @param json input json
 * @returns converted rofi world
 */
inline auto getRofiWorldFromJson( const nlohmann::json & json )
        -> atoms::Result< rofi::configuration::RofiWorld >
{
    using namespace std::string_literals;
    try {
        return atoms::result_value( rofi::configuration::serialization::fromJSON( json ) );
    } catch ( const std::exception & e ) {
        return atoms::result_error( "Error while converting json to value: "s + e.what() );
    }
}

/**
 * @brief Converts `nlohmann::json` to rofi world sequence.
 * Returns an error if converting throws an exception.
 * @param json input json
 * @returns converted rofi world sequence
 */
inline auto getRofiWorldSeqFromJson( const nlohmann::json & json )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    using namespace std::string_literals;
    if ( !json.is_array() ) {
        return atoms::result_error( "Expected an array of rofi worlds"s );
    }
    try {
        auto result = std::vector< rofi::configuration::RofiWorld >();
        result.reserve( json.size() );
        for ( auto & worldJson : json ) {
            result.push_back( rofi::configuration::serialization::fromJSON( worldJson ) );
        }
        return atoms::result_value( std::move( result ) );
    } catch ( const std::exception & e ) {
        return atoms::result_error( "Error while converting json to value: "s + e.what() );
    }
}

/**
 * @brief Converts rofi world sequence to `nlohmann::json`.
 * @param rofiWorldSeq input rofi world sequence
 * @returns converted json
 */
inline auto convertRofiWorldSeqToJson(
        std::span< const rofi::configuration::RofiWorld > rofiWorldSeq ) -> nlohmann::json
{
    auto rofiWorldSeqJson = nlohmann::json::array();
    for ( const auto & rofiWorld : rofiWorldSeq ) {
        rofiWorldSeqJson.push_back( rofi::configuration::serialization::toJSON( rofiWorld ) );
    }
    return rofiWorldSeqJson;
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

} // namespace rofi::parsing
