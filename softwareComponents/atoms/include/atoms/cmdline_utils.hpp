#pragma once

#include <concepts>
#include <filesystem>
#include <fstream>
#include <iostream>


namespace atoms
{

/**
 * @brief Reads input from source \p inputFilePath .
 * If the value of \p inputFilePath is `-`, `std::cin` is used as the source.
 * Otherwise the value is treated as a file name.
 * @param inputFilePath path to file containing the input
 * @throws if the file \p inputFilePath could not be opened
 * @returns the result of invocation of `readCallback`
 */
auto readInput( const std::filesystem::path & inputFilePath,
                std::invocable< std::istream & > auto readCallback )
{
    if ( inputFilePath == "-" ) {
        return readCallback( std::cin );
    } else {
        auto inputFile = std::ifstream( inputFilePath );
        if ( !inputFile.is_open() ) {
            throw std::runtime_error( "Cannot open input file '" + inputFilePath.generic_string()
                                      + "'" );
        }
        return readCallback( inputFile );
    }
}

/**
 * @brief Writes output to destination \p outputFilePath .
 * If the value of \p outputFilePath is `-`, `std::cout` is used as
 * the destination. Otherwise the value is treated as a file name.
 * @param outputFilePath path to file containing the output
 * @throws if the file \p outputFilePath could not be opened
 * @returns the result of invocation of `writeCallback`
 */
auto writeOutput( const std::filesystem::path & outputFilePath,
                  std::invocable< std::ostream & > auto writeCallback )
{
    if ( outputFilePath == "-" ) {
        return writeCallback( std::cout );
    } else {
        auto outputFile = std::ofstream( outputFilePath );
        if ( !outputFile.is_open() ) {
            throw std::runtime_error( "Cannot open output file '" + outputFilePath.generic_string()
                                      + "'" );
        }
        return writeCallback( outputFile );
    }
}

} // namespace atoms
