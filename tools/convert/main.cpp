#include <cassert>
#include <iostream>

#include <atoms/cmdline_utils.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing.hpp>


void convert( Dim::Cli & cli );

static auto command = Dim::Cli().command( "" ).action( convert ).desc( "Convert rofi worlds" );


static auto & inputWorldFile = command.opt< std::filesystem::path >( "<input_world_file>" )
                                       .defaultDesc( {} )
                                       .desc( "Input world file ('-' for standard input)" );
static auto & outputWorldFile = command.opt< std::filesystem::path >( "<output_world_file>" )
                                        .defaultDesc( {} )
                                        .desc( "Output world file ('-' for standard output)" );

static auto & inputWorldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "if input-format" )
                                         .valueDesc( "input_world_format" )
                                         .desc( "Format of the input world file" )
                                         .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
                                         .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
                                         .choice( rofi::parsing::RofiWorldFormat::Old, "old" );
static auto & outputWorldFormat = command.opt< rofi::parsing::RofiWorldFormat >(
                                                 "of output-format" )
                                          .valueDesc( "output_world_format" )
                                          .desc( "Format of the output world file" )
                                          .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
                                          .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
                                          .choice( rofi::parsing::RofiWorldFormat::Old, "old" );

static auto & sequence = command.opt< bool >( "seq sequence" )
                                 .desc( "Convert an array of worlds (default is a single world)" );
static auto & byOne = command.opt< bool >( "b by-one" )
                              .desc( "Fixate all modules by themselves"
                                     " - no roficom connections will be made"
                                     " (only applicable for voxel format)" )
                              .after( []( auto & cli, auto & opt, auto & ) {
                                  if ( *opt ) {
                                      if ( *inputWorldFormat
                                           != rofi::parsing::RofiWorldFormat::Voxel ) {
                                          cli.badUsage(
                                                  "`by-one` is only applicable for voxel format" );
                                          return false;
                                      }
                                  }
                                  return true;
                              } );


void convertSingleWorld( Dim::Cli & cli )
{
    auto rofiWorld = atoms::readInput( *inputWorldFile, []( std::istream & istr ) {
        return rofi::parsing::parseRofiWorld( istr, *inputWorldFormat, *byOne );
    } );
    if ( !rofiWorld ) {
        cli.fail( EXIT_FAILURE, "Error while reading input", rofiWorld.assume_error() );
        return;
    }

    if ( auto valid = rofiWorld->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Invalid world", valid.assume_error() );
        return;
    }

    auto result = atoms::writeOutput( *outputWorldFile, [ &rofiWorld ]( std::ostream & ostr ) {
        return rofi::parsing::writeRofiWorld( ostr, *rofiWorld, *outputWorldFormat );
    } );
    if ( !result ) {
        cli.fail( EXIT_FAILURE, "Error while writing world", result.assume_error() );
        return;
    }
}

void convertWorldSequence( Dim::Cli & cli )
{
    auto rofiWorldSeq = atoms::readInput( *inputWorldFile, []( std::istream & istr ) {
        return rofi::parsing::parseRofiWorldSeq( istr, *inputWorldFormat, *byOne );
    } );
    if ( !rofiWorldSeq ) {
        cli.fail( EXIT_FAILURE, "Error while reading input sequence", rofiWorldSeq.assume_error() );
        return;
    }

    if ( auto valid = rofi::parsing::validateRofiWorldSeq( *rofiWorldSeq ); !valid ) {
        static_assert( std::is_same_v< decltype( valid ), atoms::Result< std::monostate > > );
        cli.fail( EXIT_FAILURE, "Invalid world sequence", valid.assume_error() );
        return;
    }

    auto result = atoms::writeOutput( *outputWorldFile, [ &rofiWorldSeq ]( std::ostream & ostr ) {
        return rofi::parsing::writeRofiWorldSeq( ostr, *rofiWorldSeq, *outputWorldFormat );
    } );
    if ( !result ) {
        cli.fail( EXIT_FAILURE, "Error while writing world sequence", result.assume_error() );
        return;
    }
}


void convert( Dim::Cli & cli )
{
    if ( *inputWorldFormat == *outputWorldFormat ) {
        if ( !inputWorldFormat && !outputWorldFormat ) {
            cli.badUsage( "Specify format of input or output world file"
                          " (by `input-format` or `output-format`)" );
            return;
        }
    }

    if ( *sequence ) {
        convertWorldSequence( cli );
    } else {
        convertSingleWorld( cli );
    }
}


int main( int argc, char * argv[] )
{
    return Dim::Cli().exec( std::cerr, argc, argv );
}
