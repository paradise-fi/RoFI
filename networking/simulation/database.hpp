#pragma once

#include <map>

#include "internal_state.hpp"


namespace rofi::networking
{
class Database
{
public:
    using RofiId = InternalState::RofiId;

    auto begin()
    {
        return _internalStates.begin();
    }
    auto begin() const
    {
        return _internalStates.begin();
    }
    auto cbegin() const
    {
        return _internalStates.begin();
    }
    auto end()
    {
        return _internalStates.end();
    }
    auto end() const
    {
        return _internalStates.end();
    }
    auto cend() const
    {
        return _internalStates.end();
    }

    bool hasRofi( RofiId rofiId ) const
    {
        return _internalStates.find( rofiId ) != _internalStates.end();
    }

private:
    std::map< RofiId, InternalState > _internalStates;
};

} // namespace rofi::networking
