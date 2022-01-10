#pragma once

#include <simplesim/controllers.hpp>


namespace rofi::simplesim
{
inline Controller runSimplesim(
        std::shared_ptr< const rofi::configuration::Rofibot > rofibotConfiguration,
        Controller::OnConfigurationUpdate onConfigurationUpdate )
{
    auto simulation = std::make_shared< Simulation >( std::move( rofibotConfiguration ) );
    auto communication = std::make_shared< Communication >( simulation->commandHandler() );

    return Controller::runRofiController( std::move( simulation ),
                                          std::move( communication ),
                                          std::move( onConfigurationUpdate ) );
}

} // namespace rofi::simplesim
