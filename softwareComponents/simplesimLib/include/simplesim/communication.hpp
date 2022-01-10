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
    // Returns false if the moduleId was already registered
    bool addNewModule( ModuleId moduleId )
    {
        return _modules.addNewModule( moduleId );
    }

    template < typename ResponsesContainer >
    void sendRofiResponses( ResponsesContainer && responses )
    {
        _modules.sendRofiResponses( std::forward< ResponsesContainer >( responses ) );
    }

    gazebo::transport::NodePtr node() const
    {
        return _node;
    }

private:
    const std::string _worldName;
    gazebo::transport::NodePtr _node;

    ModulesCommunication _modules;
};

} // namespace rofi::simplesim
