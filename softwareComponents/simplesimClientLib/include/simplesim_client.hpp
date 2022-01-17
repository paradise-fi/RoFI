#pragma once

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "atoms/guarded.hpp"
#include "configuration/rofibot.hpp"

#include <simplesim_settings_cmd.pb.h>


#define vtkTypeMacro_( thisClass, superClass ) \
    vtkTypeMacro( thisClass, superClass ) static_assert( true, "require semicolon" )

namespace rofi::simplesim
{
namespace detail
{
    class ModuleRenderInfo
    {
    public:
        std::vector< vtkSmartPointer< vtkActor > > componentActors;
        std::unordered_set< int > activeConnectors;
    };

} // namespace detail


// TODO use the same mapper for all modules
// TODO use the same property for setting the modules
class SimplesimClient
{
private:
    class UpdateConfigurationCommand : public vtkCommand
    {
        vtkTypeMacro_( UpdateConfigurationCommand, vtkCommand );

    public:
        static UpdateConfigurationCommand * New()
        {
            return new UpdateConfigurationCommand();
        }

        void Execute( vtkObject * /* caller */, unsigned long /* eventId */, void * /* callData */ )
        {
            assert( client );

            client->renderCurrentConfiguration();
        }

        SimplesimClient * client = {};
    };

public:
    using OnSettingsCmdCallback = std::function< void( const msgs::SettingsCmd & ) >;

    SimplesimClient( OnSettingsCmdCallback onSettingsCmdCallback = {} );

    void setOnSettingsCmdCallback( OnSettingsCmdCallback onSettingsCmdCallback )
    {
        assert( onSettingsCmdCallback );
        assert( !_onSettingsCmdCallback );
        _onSettingsCmdCallback = std::move( onSettingsCmdCallback );
    }

    // Blocks until the user closes the window
    void run()
    {
        assert( _onSettingsCmdCallback && "Set settings cmd callback before running" );
        if ( !_onSettingsCmdCallback ) {
            _onSettingsCmdCallback = []( auto && ) {};
        }

        assert( _renderWindow.Get() != nullptr );
        assert( _renderWindowInteractor.Get() != nullptr );

        renderCurrentConfiguration();

        _renderWindowInteractor->Start();
    }

    // Can be called from any thread
    void onConfigurationUpdate(
            std::shared_ptr< const rofi::configuration::Rofibot > newConfiguration )
    {
        assert( newConfiguration );
        assert( newConfiguration->isValid( rofi::configuration::SimpleCollision() ).first );
        _currentConfiguration.visit( [ &newConfiguration ]( auto & currConfig ) {
            currConfig = std::move( newConfiguration );
        } );
    }

    // Can be called from any thread
    void onSettingsResponse( const msgs::SettingsState & settingsState )
    {
        _currentSettings.visit(
                [ &settingsState ]( auto & currentSettings ) { currentSettings = settingsState; } );
    }

private:
    std::shared_ptr< const rofi::configuration::Rofibot > getCurrentConfig() const
    {
        return _currentConfiguration.visit( [ this ]( const auto & config ) { return config; } );
    }

    msgs::SettingsState getCurrentSettings() const
    {
        return _currentSettings.visit( [ this ]( const auto & settings ) { return settings; } );
    }

    void clearRenderer();
    void renderCurrentConfiguration();

    /// Explicit request to send the current configuration and settings state.
    ///
    /// Do not use this every frame, the server will send both automatically on change.
    void sendConfigurationAndState()
    {
        onSettingsCmd( createSettingsCmd( msgs::SettingsCmd::SEND_CONFIGURATION_AND_STATE ) );
    }
    /// Pause the simulation.
    ///
    /// You the client should continue to display the updated configuration even when paused.
    void pause()
    {
        onSettingsCmd( createSettingsCmd( msgs::SettingsCmd::PAUSE ) );
    }
    /// Resume the simulation.
    void resume()
    {
        onSettingsCmd( createSettingsCmd( msgs::SettingsCmd::RESUME ) );
    }
    /// Change the simulation time to real time ratio.
    void changeSpeedRatio( float newRatio )
    {
        assert( newRatio >= 0.f );
        auto cmd = createSettingsCmd( msgs::SettingsCmd::CHANGE_SPEED_RATIO );
        cmd.set_sim_speed_ratio( newRatio );
        onSettingsCmd( cmd );
    }
    /// Change the simulation step time.
    /// (How much milliseconds passes in simulation on each tick.)
    void changeSimStepTime( int newStepTimeMs )
    {
        assert( newStepTimeMs > 0 );
        auto cmd = createSettingsCmd( msgs::SettingsCmd::CHANGE_SIM_STEP_TIME );
        cmd.set_sim_step_time_ms( newStepTimeMs );
        onSettingsCmd( cmd );
    }

    void onSettingsCmd( const msgs::SettingsCmd & cmd )
    {
        assert( _onSettingsCmdCallback );
        _onSettingsCmdCallback( cmd );
    }
    static msgs::SettingsCmd createSettingsCmd( msgs::SettingsCmd::Type cmdType )
    {
        auto settingsCmd = msgs::SettingsCmd();
        settingsCmd.set_cmd_type( cmdType );
        return settingsCmd;
    }


    atoms::Guarded< msgs::SettingsState > _currentSettings;

    atoms::Guarded< std::shared_ptr< const rofi::configuration::Rofibot > > _currentConfiguration;
    std::shared_ptr< const rofi::configuration::Rofibot > _lastRenderedConfiguration;

    vtkNew< vtkRenderer > _renderer;
    vtkNew< vtkRenderWindow > _renderWindow;
    vtkNew< vtkInteractorStyleTrackballCamera > _interactorStyle;
    vtkNew< UpdateConfigurationCommand > _updateConfigurationCommand;
    vtkNew< vtkRenderWindowInteractor > _renderWindowInteractor;

    std::map< rofi::configuration::ModuleId, detail::ModuleRenderInfo > _moduleRenderInfos;

    OnSettingsCmdCallback _onSettingsCmdCallback;
};

} // namespace rofi::simplesim
