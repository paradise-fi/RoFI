#pragma once

#include <concepts>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string_view>


namespace atoms
{

/**
 * @brief Reads input from source \p inputFilePath .
 * If the value of \p inputFilePath is `-`, `std::cin` is used as the source.
 * Otherwise the value is treated as a file name.
 * @param inputFilePath path to file containing the input
 * @param readCallback callback to read data from input
 * @throws if the file \p inputFilePath could not be opened
 *      or if \p readCallback throws
 * @returns the result of invocation of \p readCallback
 */
auto readInput( const std::filesystem::path & inputFilePath,
                std::invocable< std::istream & > auto readCallback )
{
    if ( inputFilePath == "-" ) {
        return readCallback( std::cin );
    } else {
        auto inputFile = std::ifstream( inputFilePath );
        if ( !inputFile.is_open() ) {
            throw std::runtime_error( "Cannot open input file '" + inputFilePath.string() + "'" );
        }
        return readCallback( inputFile );
    }
}

/**
 * @brief Writes output to destination \p outputFilePath .
 * If the value of \p outputFilePath is `-`, `std::cout` is used as
 * the destination. Otherwise the value is treated as a file name.
 * @param outputFilePath path to file containing the output
 * @param writeCallback callback to write data to output
 * @throws if the file \p outputFilePath could not be opened
 *      or if \p writeCallback throws
 * @returns the result of invocation of \p writeCallback
 */
auto writeOutput( const std::filesystem::path & outputFilePath,
                  std::invocable< std::ostream & > auto writeCallback )
{
    if ( outputFilePath == "-" ) {
        return writeCallback( std::cout );
    } else {
        auto outputFile = std::ofstream( outputFilePath );
        if ( !outputFile.is_open() ) {
            throw std::runtime_error( "Cannot open output file '" + outputFilePath.string() + "'" );
        }
        return writeCallback( outputFile );
    }
}


/**
 * @brief Reads input to string from source \p inputFilePath .
 * If the value of \p inputFilePath is `-`, `std::cin` is used as the source.
 * Otherwise the value is treated as a file name.
 * @param inputFilePath path to file containing the input
 * @throws if the file \p inputFilePath could not be opened
 * @returns the input data from \p inputFilePath
 */
inline auto readInputToString( const std::filesystem::path & inputFilePath )
{
    return atoms::readInput( inputFilePath, []( std::istream & istr ) {
        return std::string( std::istreambuf_iterator( istr ), {} );
    } );
}

/**
 * @brief Writes \p writeData to destination \p outputFilePath .
 * If the value of \p outputFilePath is `-`, `std::cout` is used as
 * the destination. Otherwise the value is treated as a file name.
 * @param outputFilePath path to file containing the output
 * @param writeData data to write to output
 * @throws if the file \p outputFilePath could not be opened
 */
inline void writeOutputFromString( const std::filesystem::path & outputFilePath,
                                   std::string_view writeData )
{
    writeOutput( outputFilePath,
                 [ & ]( std::ostream & ostr ) { ostr << writeData << std::flush; } );
}

} // namespace atoms
