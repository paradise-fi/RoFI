#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <vector>

#include <rofiResp.pb.h>


namespace rofi::simplesim {

template < typename Data >
class DelayedDataHandler {
    using TimeSinceStart = std::chrono::milliseconds;

public:
    std::vector< Data > advanceTime( std::chrono::milliseconds duration )
    {
        _timeSinceStart += duration;

        auto begin = _waitInfos.begin();
        auto end = _waitInfos.upper_bound( _timeSinceStart );

        std::vector< Data > result;
        std::transform( begin, end, std::back_inserter( result ), []( auto&& p ) {
            return std::move( p.second );
        } );
        _waitInfos.erase( begin, end );
        return result;
    }

    void registerDelayedData( Data data, std::chrono::milliseconds duration )
    {
        _waitInfos.emplace( _timeSinceStart + duration, std::move( data ) );
    }

private:
    TimeSinceStart _timeSinceStart = TimeSinceStart::zero();

    std::multimap< TimeSinceStart, Data > _waitInfos;
};

} // namespace rofi::simplesim
