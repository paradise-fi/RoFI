#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <future>

#include <gazebo/gazebo_client.hh>
#include <atoms/units.hpp>

#include "rofi_hal.hpp"

enum class JointCmd
{
    GET_CAPABILITIES,
    GET_VELOCITY,
    SET_VELOCITY,
    GET_POSITION,
    SET_POSITION,
    GET_TORQUE,
    SET_TORQUE,
};

enum class ConnectorCmd
{
    RETRACT,
    EXTEND,
    CONNECT,
    DISCONNECT,
    GET_STATE,
    PACKET,
    CONNECT_POWER,
    DISCONNECT_POWER,
};

inline std::string_view ltrim( std::string_view line, std::string_view ws = " \t" )
{
    auto first = line.find_first_not_of( ws );
    return first == std::string_view::npos ? "" : line.substr( first );
}

inline std::string_view rtrim( std::string_view line, std::string_view ws = " \t" )
{
    auto last = line.find_last_not_of( ws );
    return last == std::string_view::npos ? "" : line.substr( 0, last + 1 );
}

inline std::string_view trim( std::string_view line, std::string_view ws = " \t" )
{
    return rtrim( ltrim( line, ws ), ws );
}

inline std::vector< std::string_view > split( std::string_view line )
{
    const std::string_view ws = " \t";

    line = trim( line, ws );
    std::vector< std::string_view > tokens;
    while ( !line.empty() && line.front() != '#' )
    {
        auto last = line.find_first_of( ws );
        tokens.push_back( line.substr( 0, last ) );

        auto nextFirst = line.find_first_not_of( ws, last );
        if ( nextFirst == std::string_view::npos )
        {
            break;
        }

        line.remove_prefix( nextFirst );
    }

    return tokens;
}

float readFloat( std::string_view str )
{
    return float( std::atof( std::string( str ).data() ) );
}

int readInt( std::string_view str )
{
    return std::atoi( std::string( str ).data() );
}

std::string toString( rofi::hal::ConnectorPosition position )
{
    using rofi::hal::ConnectorPosition;

    switch ( position )
    {
        case ConnectorPosition::Retracted:
        {
            return "retracted";
        }
        case ConnectorPosition::Extended:
        {
            return "extended";
        }
    }

    throw std::runtime_error( "Unknown connector position" );
}

std::string toString( rofi::hal::ConnectorOrientation orientation )
{
    using rofi::hal::ConnectorOrientation;

    switch ( orientation )
    {
        case ConnectorOrientation::North:
        {
            return "north";
        }
        case ConnectorOrientation::East:
        {
            return "east";
        }
        case ConnectorOrientation::South:
        {
            return "south";
        }
        case ConnectorOrientation::West:
        {
            return "west";
        }
    }

    throw std::runtime_error( "Unknown connector orientation" );
}


class Jobs
{
private:

    Jobs() {}
    Jobs( const Jobs& ) = delete;

    std::unordered_map< int, std::promise<void> > _jobs;
    int _nextId = 0;

public:

    static Jobs& get()
    {
        static Jobs jobsInstance;
        return jobsInstance;
    }

    int startNew()
    {
        _jobs.insert( { _nextId, std::promise<void>() } );
        return _nextId++;
    }

    void finish( int job )
    {
        if ( !_jobs.contains( job ) ) throw std::runtime_error( "Job does not exist" );
        _jobs[job].set_value();
    }

    void waitFor( int job )
    {
        if ( !_jobs.contains( job ) ) throw std::runtime_error( "Job does not exist" );
        _jobs[job].get_future().get();
        _jobs.erase( job ); 
    }

    void synchronize()
    {
        for ( auto& [ _, prom ] : _jobs )
            prom.get_future().get();

        _jobs.clear();
    }

};

JointCmd getJointCmdType( std::string_view token )
{
    const std::map< std::string_view, JointCmd > map = {
        { "gc", JointCmd::GET_CAPABILITIES }, { "getcapabilities", JointCmd::GET_CAPABILITIES },
        { "gp", JointCmd::GET_POSITION },     { "getposition", JointCmd::GET_POSITION },
        { "gv", JointCmd::GET_VELOCITY },     { "getvelocity", JointCmd::GET_VELOCITY },
        { "gt", JointCmd::GET_TORQUE },       { "gettorque", JointCmd::GET_TORQUE },
        { "sp", JointCmd::SET_POSITION },     { "setposition", JointCmd::SET_POSITION },
        { "sv", JointCmd::SET_VELOCITY },     { "setvelocity", JointCmd::SET_VELOCITY },
        { "st", JointCmd::SET_TORQUE },       { "settorque", JointCmd::SET_TORQUE },
    };

    auto it = map.find( token );
    if ( it == map.end() )
        throw std::runtime_error( "Unknown joint cmd type" );
    return it->second;
}

inline ConnectorCmd getConnectorCmdType( std::string_view token )
{
    const std::map< std::string_view, ConnectorCmd > map = {
        { "gs", ConnectorCmd::GET_STATE },
        { "getstate", ConnectorCmd::GET_STATE },
        { "r", ConnectorCmd::RETRACT },
        { "retract", ConnectorCmd::RETRACT },
        { "e", ConnectorCmd::DISCONNECT },
        { "extend", ConnectorCmd::EXTEND },
        { "c", ConnectorCmd::CONNECT },
        { "connect", ConnectorCmd::CONNECT },
        { "d", ConnectorCmd::DISCONNECT },
        { "disconnect", ConnectorCmd::DISCONNECT },
        { "sp", ConnectorCmd::PACKET },
        { "sendpacket", ConnectorCmd::PACKET },
        { "cp", ConnectorCmd::CONNECT_POWER },
        { "connectpower", ConnectorCmd::CONNECT_POWER },
        { "dp", ConnectorCmd::DISCONNECT_POWER },
        { "disconnectpower", ConnectorCmd::DISCONNECT_POWER },
    };

    auto it = map.find( token );
    if ( it == map.end() )
        throw std::runtime_error( "Unknown connector cmd type" );
    return it->second;
}

void printHelp()
{
    std::cerr << "\nUsage:\n";
    std::cerr << "\tquit (q)\n";
    std::cerr << "\texec (e) <command_file>\n";
    std::cerr << "\tsynchronize (s)\n";
    std::cerr << "\tmodule (m) <module_id> <module_command>\n";

    std::cerr<< "\nModule Commands:\n";
    std::cerr << "\tdescriptor (d)\n";
    std::cerr << "\tjoint (j) <joint_number> <joint_command>\n";
    std::cerr << "\tconnector (c) <connector_number> <connector_command>\n";

    std::cerr << "\nJoint commands:\n";
    std::cerr << "\tgetcapabilities (gc)\n";
    std::cerr << "\tgetposition (gp)\n";
    std::cerr << "\tgetvelocity (gv)\n";
    std::cerr << "\tgettorque (gt)\n";
    std::cerr << "\tsetposition (sp) <position> [velocity=1]\n";
    std::cerr << "\tsetvelocity (sv) <velocity>\n";
    std::cerr << "\tsettorque (st) <torque>\n";

    std::cerr << "\nConnector commands:\n";
    std::cerr << "\tgetstate (gs)\n";
    std::cerr << "\tretract (r)\n";
    std::cerr << "\textend (e)\n";
    std::cerr << "\tconnect (c)\n";
    std::cerr << "\tdisconnect (d)\n";
    // std::cerr << "\tsendpacket (sp)\n"; // not implemented
    // std::cerr << "\tconnectpower (cp)\n"; // not implemented
    // std::cerr << "\tdisconnectpower (dp)\n"; // not implemented
}

void processCmds( 
    std::istream & commandStream,
    std::unordered_map< int, rofi::hal::RoFI > & modules, 
    std::ofstream* logFile = nullptr );

void processJointCmd( rofi::hal::RoFI & rofi, const std::vector< std::string_view > & tokens )
{
    if ( tokens.size() < 3 )
        throw std::runtime_error( "Wrong number of arguments" );

    auto joint = rofi.getJoint( readInt( tokens[ 1 ] ) );

    switch ( getJointCmdType( tokens[ 2 ] ) )
    {
        case JointCmd::GET_CAPABILITIES:
        {
            auto maxPosition = joint.maxPosition();
            auto minPosition = joint.minPosition();
            auto maxSpeed = joint.maxSpeed();
            auto minSpeed = joint.minSpeed();
            auto maxTorque = joint.maxTorque();
            std::cout << "Max position: " << maxPosition << "\n";
            std::cout << "Min position: " << minPosition << "\n";
            std::cout << "Max speed: " << maxSpeed << "\n";
            std::cout << "Min speed: " << minSpeed << "\n";
            std::cout << "Max torque: " << maxTorque << "\n";
            break;
        }
        case JointCmd::GET_VELOCITY:
        {
            auto result = joint.getVelocity();
            std::cout << "Current velocity: " << result << "\n";
            break;
        }
        case JointCmd::SET_VELOCITY:
        {
            if ( tokens.size() != 4 )
            {
                throw std::runtime_error( "Wrong number of arguments" );
            }
            joint.setVelocity( readFloat( tokens[ 3 ] ) );
            break;
        }
        case JointCmd::GET_POSITION:
        {
            auto result = joint.getPosition();
            std::cout << "Current position: " << result << "\n";
            break;
        }
        case JointCmd::SET_POSITION:
        {
            if ( tokens.size() != 4 && tokens.size() != 5 )
                throw std::runtime_error( "Wrong number of arguments" );

            float speed = 1;
            if ( tokens.size() == 5 )
                speed = readFloat( tokens[ 4 ] );

            int job = Jobs::get().startNew();
            joint.setPosition( Angle::deg( readFloat( tokens[ 3 ] ) ).rad(),
                               speed,
                               [ job ]( rofi::hal::Joint ) 
                                { 
                                    std::cout << "Position reached\n"; 
                                    Jobs::get().finish( job ); 
                                } );
            break;
        }
        case JointCmd::GET_TORQUE:
        {
            auto result = joint.getTorque();
            std::cout << "Current torque: " << result << "\n";
            break;
        }
        case JointCmd::SET_TORQUE:
        {
            if ( tokens.size() != 4 )
            {
                throw std::runtime_error( "Wrong number of arguments" );
            }
            joint.setTorque( readFloat( tokens[ 3 ] ) );
            break;
        }
    }
}

void processConnectorCmd( rofi::hal::RoFI & rofi, const std::vector< std::string_view > & tokens )
{
    if ( tokens.size() < 3 )
    {
        throw std::runtime_error( "Wrong number of arguments" );
    }

    auto connector = rofi.getConnector( readInt( tokens[ 1 ] ) );

    switch ( getConnectorCmdType( tokens[ 2 ] ) )
    {
        case ConnectorCmd::RETRACT:
        {
            auto state = connector.getState();
            if ( state.position == rofi::hal::ConnectorPosition::Retracted ) {
                std::cout << "Connector is already retracted" << std::endl;
                break;
            }

            connector.disconnect();
            std::cout << "Retracting connector" << std::endl;
            break;
        }
        case ConnectorCmd::EXTEND:
        {
            auto state = connector.getState();
            if ( state.position == rofi::hal::ConnectorPosition::Extended ) {
                std::cout << "Connector is already extended" << std::endl;
                break;
            }

            connector.connect();
            std::cout << "Extending connector" << std::endl;
            break;
        }
        case ConnectorCmd::CONNECT:
        {
            auto state = connector.getState();

            if ( state.connected )
                throw std::runtime_error( "Connector is already connected" );

            if ( state.position == rofi::hal::ConnectorPosition::Extended )
            {
                connector.disconnect();
                std::cout << "Retracting connector" << std::endl;
            }

            int job = Jobs::get().startNew();
            auto onceFlag = std::once_flag{}; // does the once flag matter?
            connector.onConnectorEvent( [ & ]( rofi::hal::Connector, rofi::hal::ConnectorEvent event ) {
                if ( event == rofi::hal::ConnectorEvent::Connected ) {
                    std::cout << "Connected" << std::endl;
                    std::call_once( onceFlag, [ & ]() { Jobs::get().finish( job ); } );
                }
            });
            connector.connect();
            std::cout << "Extending connector" << std::endl;
            Jobs::get().waitFor( job ); // assumes there is a connector to connect to, otherwise waits

            // Clear callback to avoid dangling references in lambda
            connector.onConnectorEvent( nullptr );
            break;
        }
        case ConnectorCmd::DISCONNECT:
        {
            auto state = connector.getState();

            if ( !state.connected )
                throw std::runtime_error( "Connector is already disconnected" );

            assert( state.position == rofi::hal::ConnectorPosition::Extended );

            int job = Jobs::get().startNew();
            auto onceFlag = std::once_flag{}; // does the once flag matter?
            connector.onConnectorEvent( [ & ]( rofi::hal::Connector, rofi::hal::ConnectorEvent event ) {
                if ( event == rofi::hal::ConnectorEvent::Disconnected ) {
                    std::cout << "Disconnected" << std::endl;
                    std::call_once( onceFlag, [ & ]() { Jobs::get().finish( job ); } );
                }
            });
            connector.disconnect();
            std::cout << "Retracting connector" << std::endl;
            Jobs::get().waitFor( job ); // must successfully disconnect

            // Clear callback to avoid dangling references in lambda
            connector.onConnectorEvent( nullptr );
            break;
        }
        case ConnectorCmd::GET_STATE:
        {
            auto result = connector.getState();
            std::cout << "Position: " << toString( result.position ) << "\n";
            std::cout << "Connected: " << std::boolalpha << result.connected << "\n";
            if ( result.connected )
            {
                std::cout << "Orientation: " << toString( result.orientation ) << "\n";
            }
            break;
        }
        case ConnectorCmd::PACKET:
        case ConnectorCmd::CONNECT_POWER:
        case ConnectorCmd::DISCONNECT_POWER:
        {
            std::cout << "Connector command not implemented\n";
            printHelp();
            break;
        }
    }
}

void processModuleCmd( std::unordered_map< int, rofi::hal::RoFI > & modules, std::vector< std::string_view > & tokens )
{
    if ( tokens.size() < 3 )
        throw std::runtime_error( "Wrong number of arguments" );

    int rofiId = readInt( tokens[ 1 ] );

    if ( !modules.contains( rofiId ) )
        modules.emplace( rofiId, rofi::hal::RoFI::getRemoteRoFI( rofiId ) );

    auto [ _, rofi ] = *(modules.find( rofiId ));

    tokens.erase( tokens.begin(), tokens.begin() + 2 );

    if ( tokens.front() == "d" || tokens.front() == "descriptor" )
    {
        auto descriptor = rofi.getDescriptor();
        std::cout << "Joint count: " << descriptor.jointCount << "\n";
        std::cout << "Connector count: " << descriptor.connectorCount << "\n";
        return;
    }
    
    if ( tokens.front() == "j" || tokens.front() == "joint" )
    {
        processJointCmd( rofi, tokens );
        return;
    }

    if ( tokens.front() == "c" || tokens.front() == "connector" )
    {
        processConnectorCmd( rofi, tokens );
        return;
    }

    std::cerr << "Unknown module command\n";
    printHelp();
}

void processExecCmd( std::unordered_map< int, rofi::hal::RoFI > & modules, std::vector< std::string_view > & tokens )
{
    assert( tokens[ 0 ] == "e" || tokens[ 0 ] == "exec" );
    tokens.erase( tokens.begin() );

    for ( const auto& filePath : tokens )
    {
        std::ifstream commandFile{ std::string( filePath ) };

        if ( !commandFile ) 
            throw std::runtime_error( "Command file \"" + std::string( filePath ) + "\" cannot be opened" );

        // record only the exec command itself, not all executed subcommands
        processCmds( commandFile, modules );
    }
}

void processCmds( 
    std::istream & commandStream,
    std::unordered_map< int, rofi::hal::RoFI > & modules, 
    std::ofstream* logFile )
{
    for ( std::string line; std::getline( commandStream, line ); )
    {
        try
        {
            auto tokens = split( line );

            if ( tokens.empty() )
                continue;

            if ( tokens.front() == "q" || tokens.front() == "quit" )
                break;

            if ( tokens.front() == "e" || tokens.front() == "exec" )
            {
                if ( logFile ) *logFile << line << std::endl;
                processExecCmd( modules, tokens );
                continue;
            }

            if ( tokens.front() == "m" || tokens.front() == "module" )
            {
                if ( logFile ) *logFile << line << std::endl;
                processModuleCmd( modules, tokens );
                continue;
            }

            if ( tokens.front() == "s" || tokens.front() == "synchronize" )
            {
                if ( logFile ) *logFile << line << std::endl;
                Jobs::get().synchronize();
                std::cout << "Synchronized\n"; 
                continue;
            }

            std::cerr << "Unknown command\n";
            printHelp();
        }
        catch ( const std::runtime_error & e )
        {
            std::cerr << "ERROR: " << e.what() << "\n";
        }
    }
}

int main( int argc, char ** argv )
{
    printHelp();

    try
    {
        std::ofstream logFile;

        if ( argc > 1 )
            logFile = std::ofstream( argv[ 1 ] );

        std::unordered_map< int, rofi::hal::RoFI > modules;

        processCmds( std::cin, modules, &logFile );
    }
    catch ( const gazebo::common::Exception & e )
    {
        std::cerr << e.GetErrorStr() << "\n";
    }
}
