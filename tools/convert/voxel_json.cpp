#include "voxel.hpp"

#include <nlohmann/json.hpp>

#include "common_opts.hpp"
#include "configuration/serialization.hpp"


void convertVoxelJson( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "voxel-json" )
                              .action( convertVoxelJson )
                              .desc( "Convert rofi world json to voxel json" );

static auto & inputWorldFilePath =
        command.opt< std::filesystem::path >( "<input_world_file>" )
                .defaultDesc( {} )
                .desc( "Input world file ('-' for standard input)" );
static auto & outputWorldFilePath =
        command.opt< std::filesystem::path >( "<output_world_file>" )
                .defaultDesc( {} )
                .desc( "Output world file ('-' for standard output)" );
static auto & reverse =
        command.opt< bool >( "r reverse" )
                .desc( "Convert from voxel world json to rofi world json (default is opposite)" );


void convertToVoxelJson( Dim::Cli & cli )
{
    auto inputRofiWorld = readInput( *inputWorldFilePath, readRofiWorldJsonFromStream );
    if ( !inputRofiWorld ) {
        cli.fail( EXIT_FAILURE, "Error while reading rofi world", inputRofiWorld.assume_error() );
        return;
    }
    assert( *inputRofiWorld );
    assert( ( *inputRofiWorld )->isPrepared() );
    assert( ( *inputRofiWorld )->isValid() );

    auto voxelWorld = rofi::voxel::VoxelWorld::fromRofiWorld( **inputRofiWorld );
    if ( !voxelWorld ) {
        cli.fail( EXIT_FAILURE,
                  "Error while converting rofi world to voxel world",
                  voxelWorld.assume_error() );
        return;
    }

    auto voxelWorldJson = nlohmann::json( *voxelWorld );
    writeOutput( *outputWorldFilePath, [ &voxelWorldJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << voxelWorldJson << std::endl;
    } );
}

void convertFromVoxelJson( Dim::Cli & cli )
{
    auto parseVoxelWorldFromJson =
            []( std::istream & istr ) -> atoms::Result< rofi::voxel::VoxelWorld > {
        try {
            auto inputJson = nlohmann::json::parse( istr );
            return atoms::result_value( inputJson.get< rofi::voxel::VoxelWorld >() );
        } catch ( const nlohmann::json::exception & e ) {
            using namespace std::string_literals;
            return atoms::result_error( "Error while parsing voxel world ("s + e.what() + ")" );
        }
    };
    auto inputVoxelWorld = readInput( *inputWorldFilePath, parseVoxelWorldFromJson );
    if ( !inputVoxelWorld ) {
        cli.fail( EXIT_FAILURE, "Error while parsing voxel world", inputVoxelWorld.assume_error() );
        return;
    }

    auto rofiWorld = inputVoxelWorld->toRofiWorld();
    if ( !rofiWorld ) {
        cli.fail( EXIT_FAILURE,
                  "Error while converting voxel world to rofi world",
                  rofiWorld.assume_error() );
        return;
    }
    assert( *rofiWorld );
    assert( ( *rofiWorld )->isPrepared() );
    assert( ( *rofiWorld )->isValid() );

    auto rofiWorldJson = rofi::configuration::serialization::toJSON( **rofiWorld );
    writeOutput( *outputWorldFilePath, [ &rofiWorldJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << rofiWorldJson << std::endl;
    } );
}

void convertVoxelJson( Dim::Cli & cli )
{
    if ( *reverse ) {
        convertFromVoxelJson( cli );
    } else {
        convertToVoxelJson( cli );
    }
}
