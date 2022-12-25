#include <atoms/cmdline_utils.hpp>
#include <atoms/parsing.hpp>
#include <configuration/serialization.hpp>
#include <dimcli/cli.h>


void convertOldCfg( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "old-cfg" )
                              .action( convertOldCfg )
                              .desc( "Convert old cfg (Viki) format to rofi world json" );

static auto & inputWorldFilePath = command.opt< std::filesystem::path >( "<input_world_file>" )
                                           .defaultDesc( {} )
                                           .desc( "Input world file ('-' for standard input)" );
static auto & outputWorldFilePath = command.opt< std::filesystem::path >( "<output_world_file>" )
                                            .defaultDesc( {} )
                                            .desc( "Output world file ('-' for standard output)" );


void convertFromOldCfg( Dim::Cli & cli )
{
    auto rofiWorld = atoms::readInput( *inputWorldFilePath, []( std::istream & istr ) {
                         return atoms::parseOldCfgFormat( istr, true );
                     } ).and_then( atoms::toSharedAndValidate );
    if ( !rofiWorld ) {
        cli.fail( EXIT_FAILURE, "Error while reading old configuration", rofiWorld.assume_error() );
        return;
    }
    assert( *rofiWorld );
    assert( ( *rofiWorld )->isValid() );

    auto rofiWorldJson = rofi::configuration::serialization::toJSON( **rofiWorld );
    atoms::writeOutput( *outputWorldFilePath, [ &rofiWorldJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << rofiWorldJson << std::endl;
    } );
}

void convertOldCfg( Dim::Cli & cli )
{
    convertFromOldCfg( cli );
}
