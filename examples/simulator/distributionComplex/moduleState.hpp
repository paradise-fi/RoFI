#pragma once

#include "lwip++.hpp"
#include "serializable/serializable.hpp"
#include "atoms/util.hpp"
#include <vector>
#include <string>
#include <optional>

using namespace rofi::hal;

struct ConnectorStatus
{
    int connectorId;
    std::optional< Ip6Addr > connectedTo;
    std::optional< int > otherSideConnectorId;

    ConnectorStatus() = default;
    ConnectorStatus( int connectorId, std::optional< Ip6Addr > connectedTo, std::optional< int > otherSideConnectorId )
    : connectorId( connectorId ), connectedTo( connectedTo ), otherSideConnectorId( otherSideConnectorId ) {}
};

struct JointStatus
{
    float currentAngle;
    float maxAngle;
    float minAngle;

    JointStatus() = default;
    JointStatus( float currentAngle, float maxAngle, float minAngle )
        : currentAngle( currentAngle ), maxAngle( maxAngle ), minAngle( minAngle ) {}
};

struct ModuleState : public Serializable
{
    Ip6Addr moduleAddress;
    std::vector< ConnectorStatus > connectors;
    std::vector< JointStatus > joints;

    ModuleState() : moduleAddress() {}
    ModuleState(Ip6Addr moduleAddress) : moduleAddress( moduleAddress ) {}
    ModuleState(Ip6Addr moduleAddress, std::vector< ConnectorStatus > connectors, std::vector< JointStatus > joints)
    : moduleAddress( moduleAddress ), connectors( connectors ), joints( joints ) {}

    /// @brief Copy the value of this structure into buffer. 
    /// @param buffer The buffer, it is the responsibility of the serialize() implementation to move the pointer by the size of your data type.
    virtual void serialize( uint8_t*& buffer ) const
    {
        as< Ip6Addr >( buffer ) = moduleAddress;
        size_t offset = sizeof( Ip6Addr );
        as< size_t >( buffer + offset ) = connectors.size();
        offset += sizeof( size_t );
        if ( connectors.size() > 0 )
        {
            std::memcpy( buffer + offset , connectors.data(), sizeof( ConnectorStatus ) * connectors.size() );
        }
        offset += sizeof( ConnectorStatus ) * connectors.size();
        as< size_t >( buffer + offset ) = joints.size();
        offset += sizeof( size_t );
        if ( joints.size() > 0 )
        {
            std::memcpy( buffer + offset, joints.data(), sizeof( JointStatus ) * joints.size() );
        }
    };

    /// @brief Copy the value from the buffer into this structure.
    /// @param buffer The buffer, it is the responsibility of the deserialize() implementation to move the pointer by the size of your data type.
    virtual void deserialize( const uint8_t*& buffer )
    {
        moduleAddress = as< Ip6Addr >( buffer );
        size_t offset = sizeof( Ip6Addr );
        size_t connectorsCount = as< size_t >( buffer + offset );
        offset += sizeof( size_t );
        if ( connectorsCount > 0 )
        {
            connectors.resize( connectorsCount );
            std::memcpy( connectors.data(), buffer + offset, connectorsCount * sizeof( ConnectorStatus ) );
        }
        
        offset += sizeof( ConnectorStatus ) * connectorsCount;
        size_t jointsCount = as< size_t >( buffer + offset );
        offset += sizeof( size_t );
        if ( jointsCount > 0 )
        {
            joints.resize( jointsCount );
            std::memcpy( joints.data(), buffer + offset, jointsCount * sizeof( JointStatus ) );
        }
    };

    /// @brief The size of your data type.
    /// @return size_t of the size of your data type.
    virtual std::size_t size() const
    {
        return sizeof( Ip6Addr ) + connectors.size() * sizeof( ConnectorStatus ) + joints.size() * sizeof( JointStatus ) + 2 * sizeof( std::size_t );
    }
};