#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace roficom
{
namespace detail
{
class RoFIData;
class JointData;
class ConnectorData;
} // namespace detail

class Joint
{
    // TODO class JointError {};

    friend class detail::JointData;

    detail::JointData * jointData;

    Joint( detail::JointData & data );

public:
    Joint( const Joint & ) = default;
    Joint & operator=( const Joint & ) = default;

    float maxPosition() const;
    float minPosition() const;
    float maxSpeed() const;
    float minSpeed() const;
    float maxTorque() const;
    float getVelocity() const;
    void setVelocity( float velocity );
    float getPosition() const;
    void setPosition( float pos, float speed, std::function< void( Joint ) > callback );
    float getTorque() const;
    void setTorque( float torque );
    // TODO void onError( void ( *callback )( JointError ) );
};

enum class ConnectorPosition : bool
{
    Retracted = false,
    Extended = true
};

enum class ConnectorOrientation : signed char
{
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

enum class ConnectorLine : bool
{
    Internal = 0,
    External = 1
};

struct ConnectorState
{
    ConnectorPosition position = ConnectorPosition::Retracted;
    bool internal = false;
    bool external = false;
    bool connected = false;
    ConnectorOrientation orientation = ConnectorOrientation::North;
};

struct ConnectorEvent
{
};

using Packet = std::vector< std::byte >;

class Connector
{
public:
    using State = ConnectorState;
    using Position = ConnectorPosition;
    using Orientation = ConnectorOrientation;


private:
    friend class detail::ConnectorData;

    detail::ConnectorData * connectorData;

    Connector( detail::ConnectorData & cdata );

public:
    Connector( const Connector & ) = default;
    Connector & operator=( const Connector & ) = default;

    ConnectorState getState() const;
    void connect();
    void disconnect();
    void onConnectorEvent( std::function< void( Connector, ConnectorEvent ) > callback );
    void onPacket( std::function< void( Connector, Packet ) > callback );
    void send( Packet packet );
    void connectPower( ConnectorLine );
    void disconnectPower( ConnectorLine );
};

class RoFI
{
public:
    using Id = int;

private:
    std::unique_ptr< detail::RoFIData > rofiData;

    RoFI();
    RoFI( Id remoteId );

public:
    // Destructor is default, but has to be defined in implementation (for unique_ptr)
    ~RoFI();

    RoFI( const RoFI & ) = delete;
    RoFI & operator=( const RoFI & ) = delete;

    RoFI( RoFI && ) = default;
    RoFI & operator=( RoFI && ) = default;

    static RoFI & getLocalRoFI();
    static RoFI & getRemoteRoFI( Id remoteId );

    Id getId() const;
    Joint getJoint( int index );
    Connector getConnector( int index );

    static void wait( int ms, std::function< void() > callback );
};

} // namespace roficom
