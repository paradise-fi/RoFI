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

static auto & inputWorldFilePath = command.opt< std::filesystem::path >( "<input_world_file>" )
                                           .defaultDesc( {} )
                                           .desc( "Input world file ('-' for standard input)" );
static auto & outputWorldFilePath = command.opt< std::filesystem::path >( "<output_world_file>" )
                                            .defaultDesc( {} )
                                            .desc( "Output world file ('-' for standard output)" );
static auto & reverse =
        command.opt< bool >( "r reverse" )
                .desc( "Convert from voxel world json to rofi world json (default is opposite)" );
static auto & sequence = command.opt< bool >( "seq sequence" )
                                 .desc( "Convert an array of worlds (default is single world)" );
static auto & byOne =
        command.opt< bool >( "b by-one" )
                .desc( "Fixate all modules by themselves (no roficom connections will be made)" );


auto parseVoxelWorldFromJson( std::istream & istr ) -> atoms::Result< rofi::voxel::VoxelWorld >
{
    using namespace std::string_literals;
    try {
        auto inputJson = nlohmann::json::parse( istr );
        return atoms::result_value( inputJson.get< rofi::voxel::VoxelWorld >() );
    } catch ( const nlohmann::json::exception & e ) {
        return atoms::result_error( "Error while reading voxel world ("s + e.what() + ")" );
    }
}

auto parseVoxelWorldSeqFromJson( std::istream & istr )
        -> atoms::Result< std::vector< rofi::voxel::VoxelWorld > >
{
    using namespace std::string_literals;
    try {
        auto inputJson = nlohmann::json::parse( istr );
        if ( !inputJson.is_array() ) {
            return atoms::result_error( "Expected an array of rofi worlds"s );
        }
        return atoms::result_value( inputJson.get< std::vector< rofi::voxel::VoxelWorld > >() );
    } catch ( const nlohmann::json::exception & e ) {
        return atoms::result_error( "Error while parsing voxel world sequence ("s + e.what()
                                    + ")" );
    }
}


void convertToVoxelJson( Dim::Cli & cli )
{
    auto inputRofiWorld = atoms::readInput( *inputWorldFilePath, atoms::parseRofiWorldJson )
                                  .and_then( atoms::toSharedAndValidate );
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

void convertFromVoxelJson( Dim::Cli & cli, bool separateModulesByOne )
{
    auto inputVoxelWorld = atoms::readInput( *inputWorldFilePath, parseVoxelWorldFromJson );
    if ( !inputVoxelWorld ) {
        cli.fail( EXIT_FAILURE, "Error while reading voxel world", inputVoxelWorld.assume_error() );
        return;
    }

    auto rofiWorld = inputVoxelWorld->toRofiWorld( separateModulesByOne );
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
    auto inputRofiWorldSeq = atoms::readInput( *inputWorldFilePath, atoms::parseRofiWorldSeqJson )
                                     .and_then( atoms::validateSequence );
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

void convertFromVoxelSeqJson( Dim::Cli & cli, bool separateModulesByOne )
{
    auto inputVoxelWorldSeq = atoms::readInput( *inputWorldFilePath, parseVoxelWorldSeqFromJson );
    if ( !inputVoxelWorldSeq ) {
        cli.fail( EXIT_FAILURE,
                  "Error while reading voxel world sequence",
                  inputVoxelWorldSeq.assume_error() );
        return;
    }

    assert( !inputVoxelWorldSeq->empty() );

    auto rofiWorldSeqJson = nlohmann::json::array();
    for ( size_t i = 0; i < inputVoxelWorldSeq->size(); i++ ) {
        auto rofiWorld = ( *inputVoxelWorldSeq )[ i ].toRofiWorld( separateModulesByOne );
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
    if ( *byOne && !*reverse ) {
        cli.fail( EXIT_FAILURE, "by-one flag can only be used with reverse flag" );
        return;
    }

    if ( *sequence ) {
        if ( *reverse ) {
            convertFromVoxelSeqJson( cli, *byOne );
        } else {
            convertToVoxelSeqJson( cli );
        }
    } else {
        if ( *reverse ) {
            convertFromVoxelJson( cli, *byOne );
        } else {
            convertToVoxelJson( cli );
        }
    }
}
