#pragma once

#include <chrono>
#include <memory>
#include <thread>

#include "communication.hpp"
#include "simulation.hpp"


namespace rofi::simplesim
{
class Controller
{
public:
    static constexpr std::chrono::milliseconds updateDuration = std::chrono::milliseconds( 100 );

public:
    using OnConfigurationUpdate =
            std::function< void( std::shared_ptr< const rofi::configuration::Rofibot > ) >;

    [[nodiscard]] static Controller runRofiController(
            std::shared_ptr< Simulation > simulation,
            std::shared_ptr< Communication > communication,
            OnConfigurationUpdate onConfigurationUpdate );

    void wait()
    {
        _thread.join();
    }

    void request_stop()
    {
        _thread.request_stop();
    }

private:
    Controller( std::jthread thread ) : _thread( std::move( thread ) ) {}
    static void rofiControllerThread( std::stop_token stopToken,
                                      std::shared_ptr< Simulation > simulationPtr,
                                      std::shared_ptr< Communication > communicationPtr,
                                      OnConfigurationUpdate onConfigurationUpdate );

private:
    std::jthread _thread;
};

} // namespace rofi::simplesim
