#include <fstream>
#include <stdexcept>

#include <atoms/cmdline_utils.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing.hpp>

#include "rendering.hpp"


void module_count( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "module-count" )
                              .action( module_count )
                              .desc( "Get count of modules in rofi world" );
static auto & inputWorldFile = command.opt< std::filesystem::path >( "<input_world_file>" )
                                       .defaultDesc( {} )
                                       .desc( "Input world file ('-' for standard input)" );
static auto & worldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "f format" )
                                    .valueDesc( "world_format" )
                                    .desc( "Format of the world file" )
                                    .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
                                    .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
                                    .choice( rofi::parsing::RofiWorldFormat::Old, "old" );
static auto & byOne = command.opt< bool >( "b by-one" )
                              .desc( "Fixate all modules by themselves"
                                     " - no roficom connections will be made"
                                     " (only applicable for voxel format)" )
                              .after( []( auto & cli, auto & opt, auto & ) {
                                  if ( *opt ) {
                                      if ( *worldFormat != rofi::parsing::RofiWorldFormat::Voxel ) {
                                          cli.badUsage(
                                                  "`by-one` is only applicable for voxel format" );
                                          return false;
                                      }
                                  }
                                  return true;
                              } );

void module_count( Dim::Cli & cli )
{
    auto world = atoms::readInput( *inputWorldFile, [ & ]( std::istream & istr ) {
        return rofi::parsing::parseRofiWorld( istr, *worldFormat, *byOne );
    } );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while reading input", world.assume_error() );
        return;
    }

    std::cout << world->modules().size();
}
