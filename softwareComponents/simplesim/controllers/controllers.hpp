#pragma once

#include <chrono>
#include <memory>
#include <thread>

#include "rofi_interface.hpp"
#include "simulation.hpp"


namespace rofi::simplesim
{
class Controller
{
public:
    static constexpr std::chrono::milliseconds updateDuration = std::chrono::milliseconds( 100 );

public:
    [[nodiscard]] static Controller runRofiController(
            std::shared_ptr< Simulation > simulation,
            std::shared_ptr< RofiInterface > rofiInterface );

    void wait()
    {
        _thread.join();
    }

private:
    Controller( std::jthread thread ) : _thread( std::move( thread ) ) {}
    static void rofiControllerThread( std::stop_token stopToken,
                                      std::shared_ptr< Simulation > simulationPtr,
                                      std::shared_ptr< RofiInterface > rofiInterfacePtr );

private:
    std::jthread _thread;
};

} // namespace rofi::simplesim
