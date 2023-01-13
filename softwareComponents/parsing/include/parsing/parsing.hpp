#pragma once

#include <istream>
#include <ostream>
#include <span>
#include <vector>

#include <atoms/result.hpp>
#include <atoms/unreachable.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>

#include <voxel.hpp>

#include "parsing/parsing_lite.hpp"


namespace rofi::parsing
{
namespace detail
{
    inline void printJson( std::ostream & ostr, const nlohmann::json & json )
    {
        ostr.width( 4 );
        ostr << json << std::endl;
    }
} // namespace detail


enum class RofiWorldFormat {
    Old = -1,
    Json,
    Voxel,
};

inline auto operator<<( std::ostream & ostr, RofiWorldFormat worldFormat ) -> std::ostream &
{
    switch ( worldFormat ) {
        case RofiWorldFormat::Old:
            return ostr << "old";
        case RofiWorldFormat::Json:
            return ostr << "json";
        case RofiWorldFormat::Voxel:
            return ostr << "voxel";
    }
    ROFI_UNREACHABLE( "Unknown rofi world format" );
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
inline auto parseRofiWorld( std::istream & istr,
                            RofiWorldFormat worldFormat,
                            bool fixateByOne = false )
        -> atoms::Result< rofi::configuration::RofiWorld >
{
    switch ( worldFormat ) {
        case RofiWorldFormat::Old:
            return parseOldCfgFormat( istr, true );
        case RofiWorldFormat::Json:
            return parseJson( istr ).and_then( getRofiWorldFromJson );
        case RofiWorldFormat::Voxel:
            return parseJson( istr )
                    .and_then( getFromJson< rofi::voxel::VoxelWorld > )
                    .and_then( [ & ]( auto && voxelWorld ) {
                        return voxelWorld.toRofiWorld( fixateByOne );
                    } );
    }
    ROFI_UNREACHABLE( "Unknown rofi world format" );
}

/**
 * @brief Writes rofi world to given \p ostr
 * in the format specified by \p worldFormat .
 * Returns an error if the format is incorrect
 * or if the old format is specified.
 * @param istr input stream containing the rofi world
 * @param worldFormat format of input world
 * @returns the parsed rofi world
 */
inline auto writeRofiWorld( std::ostream & ostr,
                            const rofi::configuration::RofiWorld & rofiWorld,
                            RofiWorldFormat worldFormat ) -> atoms::Result< std::monostate >
{
    assert( rofiWorld.isPrepared() );
    assert( rofiWorld.isValid() );

    switch ( worldFormat ) {
        case RofiWorldFormat::Old:
            return atoms::result_error< std::string >( "Cannot write rofi world in old format" );
        case RofiWorldFormat::Json: {
            detail::printJson( ostr, rofi::configuration::serialization::toJSON( rofiWorld ) );
            return atoms::result_value( std::monostate() );
        }
        case RofiWorldFormat::Voxel: {
            auto voxelWorld = rofi::voxel::VoxelWorld::fromRofiWorld( rofiWorld );
            if ( !voxelWorld ) {
                return voxelWorld.assume_error_result();
            }
            detail::printJson( ostr, *voxelWorld );
            return atoms::result_value( std::monostate() );
        }
    }
    ROFI_UNREACHABLE( "Unknown rofi world format" );
}


/**
 * @brief Parses rofi world sequence from given \p istr .
 * Parses the input according to \p worldFormat .
 * Returns an error if the format is incorrect.
 * @param istr input stream containing the rofi world sequence
 * @param worldFormat format of input world sequence
 * @returns the parsed rofi world sequence
 */
inline auto parseRofiWorldSeq( std::istream & istr,
                               RofiWorldFormat worldFormat,
                               bool fixateByOne = false )
        -> atoms::Result< std::vector< rofi::configuration::RofiWorld > >
{
    switch ( worldFormat ) {
        case RofiWorldFormat::Old:
            return atoms::result_error< std::string >(
                    "Parsing old format sequence is not implemented" );
        case RofiWorldFormat::Json:
            return parseJson( istr ).and_then( getRofiWorldSeqFromJson );
        case RofiWorldFormat::Voxel: {
            return parseJson( istr )
                    .and_then( getFromJson< std::vector< rofi::voxel::VoxelWorld > > )
                    .and_then( [ & ]( std::span< const rofi::voxel::VoxelWorld > voxelWorldSeq ) {
                        return convertToRofiWorldSeq( voxelWorldSeq, [ & ]( auto && voxelWorld ) {
                            return voxelWorld.toRofiWorld( fixateByOne );
                        } );
                    } );
        }
    }
    ROFI_UNREACHABLE( "Unknown rofi world format" );
}

/**
 * @brief Writes rofi world sequence to given \p ostr
 * in the format specified by \p worldFormat .
 * Returns an error if the format is incorrect
 * or if the old format is specified.
 * @param istr input stream containing the rofi world sequence
 * @param worldFormat format of input world sequence
 * @returns the parsed rofi world sequence
 */
inline auto writeRofiWorldSeq( std::ostream & ostr,
                               std::span< const rofi::configuration::RofiWorld > rofiWorldSeq,
                               RofiWorldFormat worldFormat ) -> atoms::Result< std::monostate >
{
#ifndef NDEBUG
    for ( const auto & rofiWorld : rofiWorldSeq ) {
        assert( rofiWorld.isPrepared() );
        assert( rofiWorld.isValid() );
    }
#endif

    switch ( worldFormat ) {
        case RofiWorldFormat::Old:
            return atoms::result_error< std::string >(
                    "Cannot write rofi world sequence in old format" );
        case RofiWorldFormat::Json: {
            detail::printJson( ostr, convertRofiWorldSeqToJson( rofiWorldSeq ) );
            return atoms::result_value( std::monostate() );
        }
        case RofiWorldFormat::Voxel: {
            auto voxelWorldSeq = convertFromRofiWorldSeq( rofiWorldSeq,
                                                          rofi::voxel::VoxelWorld::fromRofiWorld );
            if ( !voxelWorldSeq ) {
                return voxelWorldSeq.assume_error_result();
            }
            detail::printJson( ostr, *voxelWorldSeq );
            return atoms::result_value( std::monostate() );
        }
    }
    ROFI_UNREACHABLE( "Unknown rofi world format" );
}

} // namespace rofi::parsing
