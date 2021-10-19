#pragma once

#include <vector>

#include <boost/shared_ptr.hpp>

#include "atoms/guarded.hpp"
#include "module_states.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class CommandHandler
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;
    using RofiResp = rofi::messages::RofiResp;

    CommandHandler( std::shared_ptr< ModuleStates > moduleStates )
            : _moduleStates( std::move( moduleStates ) )
    {
        assert( _moduleStates );
    }

    std::vector< RofiCmdPtr > getRofiCommands()
    {
        return _rofiCmds.visit( []( auto & vec ) {
            auto result = std::move( vec );
            vec.clear();
            return result;
        } );
    }

    std::vector< RofiResp > processRofiCommands()
    {
        // TODO
        return {};
    }

    void onRofiCmd( const RofiCmdPtr & msg )
    {
        assert( msg );
        // TODO handle immediate messages
        _rofiCmds->push_back( msg );
    }

    auto getModuleIds()
    {
        assert( _moduleStates );
        return _moduleStates->getModuleIds();
    }

private:
    std::shared_ptr< ModuleStates > _moduleStates;

    atoms::Guarded< std::vector< RofiCmdPtr > > _rofiCmds;
};

} // namespace rofi::simplesim
