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
#include "configuration/serialization.hpp"
#include "legacy/configuration/IO.h"
#include "changecolor.hpp"

#include <QMainWindow>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QWidgetItem>

#include <simplesim_settings_cmd.pb.h>


#define vtkTypeMacro_( thisClass, superClass ) \
    vtkTypeMacro( thisClass, superClass ) static_assert( true, "require semicolon" )

namespace Ui {
    class SimplesimClient;
    class ChangeColor;
}

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
class SimplesimClient : public QMainWindow
{
    Q_OBJECT

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

    ~SimplesimClient();

    // Blocks until the user closes the window
    void run()
    {
        assert( _onSettingsCmdCallback && "Set settings cmd callback before running" );
        if ( !_onSettingsCmdCallback ) {
            _onSettingsCmdCallback = []( auto && ) {};
        }

        assert( _renderWindow.Get() != nullptr );
        assert( _renderWindowInteractor.Get() != nullptr );

        initInfoTree( *getCurrentConfig() );

        _timerId = startTimer( _simStep );
        changeSimStepTime( _simStep );

        renderCurrentConfiguration();
    }

    // Can be called from any thread
    void onConfigurationUpdate(
            std::shared_ptr< const rofi::configuration::Rofibot > newConfiguration )
    {
        assert( newConfiguration );
        assert( newConfiguration->isValid( rofi::configuration::SimpleCollision() ).first );
        _currentConfiguration.replace( std::move( newConfiguration ) );
    }

    // Can be called from any thread
    void onSettingsResponse( const msgs::SettingsState & settingsState )
    {
        _currentSettings.replace( settingsState );
    }

protected:
    void timerEvent( QTimerEvent* /* event */ );

    void colorModule( rofi::configuration::ModuleId module,
                      double color[ 3 ],
                      int component = -1 );
public slots:
    void setColor( int color );

private slots:

    void itemSelected( QTreeWidgetItem* selected );

    void changeColorWindow();

    void pauseButton();

    void speedChanged( double speed );

private:
    Ui::SimplesimClient* ui;

    void initInfoTree( const rofi::configuration::Rofibot& rofibot );

    void updateInfoTree( const rofi::configuration::Rofibot& rofibot );

    std::shared_ptr< const rofi::configuration::Rofibot > getCurrentConfig() const
    {
        return _currentConfiguration.copy();
    }

    msgs::SettingsState getCurrentSettings() const
    {
        return _currentSettings.copy();
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

    std::unique_ptr< ChangeColor > _changeColorWindow;
    vtkNew< vtkRenderer > _renderer;
    vtkNew< vtkRenderWindow > _renderWindow;
    vtkNew< vtkInteractorStyleTrackballCamera > _interactorStyle;
    vtkNew< vtkRenderWindowInteractor > _renderWindowInteractor;

    int _timerId; // ID of the timer that refreshes frames
    int _simStep = 10; // ms to refresh the frame

    int _lastModule = -1;
    double _lastColor[ 3 ];

    std::map< rofi::configuration::ModuleId, detail::ModuleRenderInfo > _moduleRenderInfos;

    OnSettingsCmdCallback _onSettingsCmdCallback;
};

} // namespace rofi::simplesim
