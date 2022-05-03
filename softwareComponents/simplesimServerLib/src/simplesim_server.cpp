#include "simplesim_server.hpp"

#include <fstream>


namespace rofi::simplesim
{

auto readConfigurationFromStream( std::istream & istr, ConfigurationFormat configFormat )
        -> rofi::configuration::Rofibot
{
    switch ( configFormat ) {
        case ConfigurationFormat::Old:
        {
            auto rofibot = configuration::readOldConfigurationFormat( istr );
            auto modules = rofibot.modules();
            if ( modules.size() != 0 ) {
                const auto & firstModule = modules.begin()->module;
                assert( firstModule.get() );
                assert( !firstModule->bodies().empty() );
                connect< configuration::RigidJoint >( firstModule->bodies().front(),
                                                      configuration::Vector( { 0, 0, 0 } ),
                                                      configuration::matrices::identity );
            }
            return rofibot;
        }
        case ConfigurationFormat::Json:
            return configuration::serialization::fromJSON( nlohmann::json::parse( istr ) );
    }
    assert( false );
    throw std::runtime_error( "Unknown configuration format" );
}

auto readAndPrepareConfigurationFromFile( const std::filesystem::path & cfgFilePath,
                                          ConfigurationFormat configFormat )
        -> std::shared_ptr< const rofi::configuration::Rofibot >
{
    auto inputCfgFile = std::ifstream( cfgFilePath );
    if ( !inputCfgFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + cfgFilePath.generic_string() + "'" );
    }

    auto rofibot = std::make_shared< configuration::Rofibot >(
            readConfigurationFromStream( inputCfgFile, configFormat ) );
    assert( rofibot );
    rofibot->prepare();
    if ( auto [ ok, str ] = rofibot->isValid( configuration::SimpleCollision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return rofibot;
}

auto readPyFilterFromFile( const std::filesystem::path & packetFilterFilePath ) -> packetf::PyFilter
{
    auto inputFile = std::ifstream( packetFilterFilePath );
    if ( !inputFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + packetFilterFilePath.generic_string()
                                  + "'" );
    }

    auto fileContent = std::string( std::istreambuf_iterator( inputFile ), {} );
    inputFile.close();

    return simplesim::packetf::PyFilter( fileContent );
}

} // namespace rofi::simplesim
