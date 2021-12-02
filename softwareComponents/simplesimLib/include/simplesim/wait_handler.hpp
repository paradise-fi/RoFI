#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <vector>

#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class WaitHandler
{
    using TimeSinceStart = std::chrono::milliseconds;

public:
    class WaitInfo
    {
    public:
        ModuleId moduleId;
        int waitId;
    };

    static rofi::messages::RofiResp getWaitResp( WaitInfo waitInfo )
    {
        rofi::messages::RofiResp rofiResp;
        rofiResp.set_rofiid( waitInfo.moduleId );
        rofiResp.set_resptype( rofi::messages::RofiCmd::WAIT_CMD );
        rofiResp.set_waitid( waitInfo.waitId );
        return rofiResp;
    }

    std::vector< WaitInfo > advanceTime( std::chrono::milliseconds duration )
    {
        _timeSinceStart += duration;

        auto begin = _waitInfos.begin();
        auto end = _waitInfos.upper_bound( _timeSinceStart );

        std::vector< WaitInfo > result;
        std::transform( begin, end, std::back_inserter( result ), []( const auto & p ) {
            return p.second;
        } );
        _waitInfos.erase( begin, end );
        return result;
    }

    void registerWait( ModuleId moduleId, int waitId, std::chrono::milliseconds duration )
    {
        _waitInfos.emplace( _timeSinceStart + duration,
                            WaitInfo{ .moduleId = moduleId, .waitId = waitId } );
    }

private:
    TimeSinceStart _timeSinceStart = TimeSinceStart::zero();

    std::multimap< TimeSinceStart, WaitInfo > _waitInfos;
};

} // namespace rofi::simplesim
