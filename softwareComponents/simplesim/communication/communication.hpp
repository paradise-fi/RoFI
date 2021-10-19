#pragma once

#include <cassert>
#include <string>

#include <gazebo/transport/transport.hh>

#include "modules_communication.hpp"


namespace rofi::simplesim
{
class Communication
{
public:
    Communication( std::shared_ptr< CommandHandler > commandHandler,
                   std::string worldName = "default" )
            : _worldName( std::move( worldName ) )
            , _node( [ this ] {
                auto node = boost::make_shared< gazebo::transport::Node >();
                assert( node );
                node->Init( this->_worldName );
                return node;
            }() )
            , _modules( std::move( commandHandler ), _node )
    {}

    // Returns true if the insertion was succesful
    // Returns false if the rofiId was already registered
    bool addNewRofi( ModulesCommunication::RofiId rofiId )
    {
        return _modules.addNewRofi( rofiId );
    }

    template < typename ResponsesContainer >
    void sendRofiResponses( ResponsesContainer && responses )
    {
        _modules.sendRofiResponses( std::forward< ResponsesContainer >( responses ) );
    }

private:
    const std::string _worldName;
    gazebo::transport::NodePtr _node;

    ModulesCommunication _modules;
};

} // namespace rofi::simplesim
