#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "atoms/guarded.hpp"
#include "module_states.hpp"
#include "wait_handler.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class CommandHandler
{
public:
    using RofiCmd = rofi::messages::RofiCmd;
    using RofiCmdPtr = boost::shared_ptr< const RofiCmd >;

    using ImmediateCmdCallback = std::function<
            std::optional< rofi::messages::RofiResp >( const ModuleStates &, const RofiCmd & ) >;
    using DelayedCmdCallback = std::function<
            std::optional< rofi::messages::RofiResp >( ModuleStates &, const RofiCmd & ) >;

    class CommandCallbacks
    {
    public:
        ImmediateCmdCallback immediate;
        DelayedCmdCallback delayed;
    };


    CommandHandler( std::shared_ptr< ModuleStates > moduleStates )
            : _moduleStates( std::move( moduleStates ) )
    {
        assert( _moduleStates );
    }

    std::optional< rofi::messages::RofiResp > onRofiCmd( const RofiCmdPtr & rofiCmdPtr );

    auto getModuleIds()
    {
        assert( _moduleStates );
        return _moduleStates->getModuleIds();
    }

    std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > extractCommandCallbacks()
    {
        return _rofiCmdCallbacks.visit( []( auto & vec ) {
            auto result = std::move( vec );
            vec.clear();
            return result;
        } );
    }

    atoms::Guarded< WaitHandler > & waitHandler()
    {
        return _waitHandler;
    }
    const atoms::Guarded< WaitHandler > & waitHandler() const
    {
        return _waitHandler;
    }

private:
    void onWaitCmd( const RofiCmd & rofiCmd );
    CommandCallbacks onRofiCmdCallbacks( const RofiCmd & cmd );

    atoms::Guarded< WaitHandler > _waitHandler;
    std::shared_ptr< const ModuleStates > _moduleStates;

    atoms::Guarded< std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > > _rofiCmdCallbacks;
};

} // namespace rofi::simplesim
