#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stop_token>
#include <thread>

#include <atoms/guarded.hpp>

#include "communication.hpp"
#include "simulation.hpp"

#include <simplesim_settings_cmd.pb.h>


namespace rofi::simplesim
{
class ServerSettings {
public:
    bool isPaused() const
    {
        return _paused;
    }
    float getSimSpeedRatio() const
    {
        assert( _simSpeedRatio >= 0.f );
        return _simSpeedRatio;
    }
    std::chrono::milliseconds getSimStepTime() const
    {
        assert( _simStepTimeMs > 0 );
        return std::chrono::milliseconds( _simStepTimeMs );
    }
    std::chrono::nanoseconds getRealStepTime() const
    {
        static constexpr size_t precision = 65536;
        if ( getSimSpeedRatio() > 0.f ) {
            return std::chrono::nanoseconds( getSimStepTime() ) * precision
                 / static_cast< size_t >( getSimSpeedRatio() * precision );
        }
        return {};
    }

    void pause()
    {
        _paused = true;
    }
    void resume()
    {
        _paused = false;
    }
    void setSimSpeedRatio( float newSpeedRatio )
    {
        assert( newSpeedRatio >= 0.f );
        _simSpeedRatio = newSpeedRatio;
    }
    void setSimStepTimeMs( int64_t newStepTimeMs )
    {
        assert( newStepTimeMs > 0 );
        _simStepTimeMs = newStepTimeMs;
    }


    msgs::SettingsState getStateMsg() const
    {
        auto msg = msgs::SettingsState();
        msg.set_paused( _paused );
        msg.set_sim_speed_ratio( _simSpeedRatio );
        msg.set_sim_step_time_ms( _simStepTimeMs );
        return msg;
    }

private:
    bool _paused = false;
    float _simSpeedRatio = 1.f;
    int64_t _simStepTimeMs = 100; // [ms]
};


class Simplesim {
public:
    using OnConfigurationUpdate =
            std::function< void( std::shared_ptr< const rofi::configuration::RofiWorld > ) >;

    Simplesim( std::shared_ptr< const rofi::configuration::RofiWorld > worldConfiguration,
               PacketFilter::FilterFunction packetFilter,
               bool verbose )
            : _simulation( std::make_shared< Simulation >( std::move( worldConfiguration ),
                                                           std::move( packetFilter ),
                                                           verbose ) )
            , _communication(
                      std::make_shared< Communication >( _simulation->commandHandler(), verbose ) )
    {
        assert( _simulation );
        assert( _communication );
    }

    std::shared_ptr< Communication > communication() const
    {
        return _communication;
    }

    void run( OnConfigurationUpdate onConfigurationUpdate, std::stop_token stopToken = {} );

    // Can be called from any thread
    [[nodiscard]] ServerSettings onSettingsCmd( const msgs::SettingsCmd & settingsCmd );

private:
    atoms::Guarded< ServerSettings > _settings;
    std::atomic_flag _configurationProvided;

    const std::shared_ptr< Simulation > _simulation;
    const std::shared_ptr< Communication > _communication;
};

} // namespace rofi::simplesim
