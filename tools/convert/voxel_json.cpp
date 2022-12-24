#include <atoms/cmdline_utils.hpp>
#include <atoms/parsing.hpp>
#include <configuration/serialization.hpp>
#include <dimcli/cli.h>
#include <nlohmann/json.hpp>

#include <voxel.hpp>


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
static auto & sequence = command.opt< bool >( "seq sequence" )
                                 .desc( "Convert an array of worlds (default is single world)" );


void convertToVoxelJson( Dim::Cli & cli )
{
    auto inputRofiWorld = atoms::readInput( *inputWorldFilePath,
                                            atoms::parseAndValidateRofiWorldJson );
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
    atoms::writeOutput( *outputWorldFilePath, [ &voxelWorldJson ]( std::ostream & ostr ) {
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
    auto inputVoxelWorld = atoms::readInput( *inputWorldFilePath, parseVoxelWorldFromJson );
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
    atoms::writeOutput( *outputWorldFilePath, [ &rofiWorldJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << rofiWorldJson << std::endl;
    } );
}

void convertToVoxelSeqJson( Dim::Cli & cli )
{
    auto inputRofiWorldSeq = atoms::readInput( *inputWorldFilePath,
                                               atoms::parseAndValidateRofiWorldSeqJson );
    if ( !inputRofiWorldSeq ) {
        cli.fail( EXIT_FAILURE,
                  "Error while reading rofi world sequence",
                  inputRofiWorldSeq.assume_error() );
        return;
    }
    assert( !inputRofiWorldSeq->empty() );

    auto voxelWorldSeq = std::vector< rofi::voxel::VoxelWorld >();
    voxelWorldSeq.reserve( inputRofiWorldSeq->size() );
    for ( size_t i = 0; i < inputRofiWorldSeq->size(); i++ ) {
        const auto & inputRofiWorld = ( *inputRofiWorldSeq )[ i ];
        assert( inputRofiWorld.isPrepared() );
        assert( inputRofiWorld.isValid() );

        auto voxelWorld = rofi::voxel::VoxelWorld::fromRofiWorld( inputRofiWorld );
        if ( !voxelWorld ) {
            cli.fail( EXIT_FAILURE,
                      "Error while converting rofi world " + std::to_string( i )
                              + " to voxel world",
                      voxelWorld.assume_error() );
            return;
        }
        voxelWorldSeq.push_back( std::move( *voxelWorld ) );
    }

    auto voxelWorldSeqJson = nlohmann::json( voxelWorldSeq );
    atoms::writeOutput( *outputWorldFilePath, [ &voxelWorldSeqJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << voxelWorldSeqJson << std::endl;
    } );
}

void convertFromVoxelSeqJson( Dim::Cli & cli )
{
    auto parseVoxelWorldSeqFromJson =
            []( std::istream & istr ) -> atoms::Result< std::vector< rofi::voxel::VoxelWorld > > {
        try {
            auto inputJson = nlohmann::json::parse( istr );
            if ( !inputJson.is_array() ) {
                return atoms::result_error< std::string >( "Expected an array of rofi worlds" );
            }
            return atoms::result_value( inputJson.get< std::vector< rofi::voxel::VoxelWorld > >() );
        } catch ( const nlohmann::json::exception & e ) {
            using namespace std::string_literals;
            return atoms::result_error( "Error while parsing voxel world sequence ("s + e.what()
                                        + ")" );
        }
    };
    auto inputVoxelWorldSeq = atoms::readInput( *inputWorldFilePath, parseVoxelWorldSeqFromJson );
    if ( !inputVoxelWorldSeq ) {
        cli.fail( EXIT_FAILURE,
                  "Error while parsing voxel world sequence",
                  inputVoxelWorldSeq.assume_error() );
        return;
    }

    assert( !inputVoxelWorldSeq->empty() );

    auto rofiWorldSeqJson = nlohmann::json::array();
    for ( size_t i = 0; i < inputVoxelWorldSeq->size(); i++ ) {
        auto rofiWorld = ( *inputVoxelWorldSeq )[ i ].toRofiWorld();
        if ( !rofiWorld ) {
            cli.fail( EXIT_FAILURE,
                      "Error while converting voxel world " + std::to_string( i )
                              + " to rofi world",
                      rofiWorld.assume_error() );
            return;
        }
        assert( *rofiWorld );
        assert( ( *rofiWorld )->isPrepared() );
        assert( ( *rofiWorld )->isValid() );

        rofiWorldSeqJson.push_back( rofi::configuration::serialization::toJSON( **rofiWorld ) );
    }

    atoms::writeOutput( *outputWorldFilePath, [ &rofiWorldSeqJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << rofiWorldSeqJson << std::endl;
    } );
}

void convertVoxelJson( Dim::Cli & cli )
{
    if ( *sequence ) {
        if ( *reverse ) {
            convertFromVoxelSeqJson( cli );
        } else {
            convertToVoxelSeqJson( cli );
        }
    } else {
        if ( *reverse ) {
            convertFromVoxelJson( cli );
        } else {
            convertToVoxelJson( cli );
        }
    }
}
