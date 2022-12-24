#include <fstream>
#include <stdexcept>

#include <atoms/parsing.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>

#include "rendering.hpp"


void preview_seq( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "preview-seq" )
                              .action( preview_seq )
                              .desc( "Interactively preview a rofi world sequence" );
static auto & inputFile = command.opt< std::string >( "<worlds_file>" )
                                  .desc( "Specify rofi world sequence source file" );

void preview_seq( Dim::Cli & cli )
{
    auto istr = std::ifstream( *inputFile );
    if ( !istr.is_open() ) {
        cli.fail( EXIT_FAILURE, "Cannot open file '" + *inputFile + "'" );
        return;
    }

    auto worlds = atoms::parseRofiWorldSeqJson( istr );
    if ( !worlds ) {
        cli.fail( EXIT_FAILURE, "Error while parsing world sequence", worlds.assume_error() );
        return;
    }

    if ( worlds->empty() ) {
        cli.fail( EXIT_FAILURE, "No rofi world in sequence" );
        return;
    }

    for ( size_t i = 0; i < worlds->size(); i++ ) {
        if ( ( *worlds )[ i ].modules().empty() ) {
            cli.fail( EXIT_FAILURE, "Empty rofi world " + std::to_string( i ) );
            return;
        }
    }

    for ( size_t i = 0; i < worlds->size(); i++ ) {
        if ( ( *worlds )[ i ].referencePoints().empty() ) {
            std::cout << "No reference points found, fixing the world " + std::to_string( i )
                                 + " in space\n";
            atoms::fixateRofiWorld( ( *worlds )[ i ] );
        }

        if ( auto valid = ( *worlds )[ i ].validate(); !valid ) {
            cli.fail( EXIT_FAILURE,
                      "Invalid rofi world " + std::to_string( i ),
                      valid.assume_error() );
            return;
        }
    }

    renderRofiWorldSequence( *worlds, "Preview of sequence " + *inputFile );
}
