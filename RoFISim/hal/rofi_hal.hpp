#pragma once
#include <functional>
#include <memory>

#include "networking.hpp"

namespace rofi::hal
{
/**
 * \brief Proxy for controlling a single joint of RoFI.
 *
 * Joint cannot be instantiated on its own; you can obtain its instance from RoFI::getJoint().
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class Joint
{
public:
    /**
     * \brief
     */
    enum class Error
    {
        Communication, ///< Cannot communicate with the servo motor.
        Hardware,      ///< Servo motor responded with an error.
    };

    /**
     * \brief Interface of the actual implementation - either physical local, physical remote or a
     * simulation one.
     *
     * To provide an implementation, simply inherit from this class.
     *
     * For method description, see description of Joint.
     */
    class Implementation : public std::enable_shared_from_this< Implementation >
    {
    protected:
        ~Implementation() = default;

    public:
        struct Capabilities
        {
            float maxPosition, minPosition;
            float maxSpeed, minSpeed;
            float maxTorque;
        };

        virtual const Capabilities & getCapabilities() = 0;
        virtual float getVelocity() = 0;
        virtual void setVelocity( float velocity ) = 0;
        virtual float getPosition() = 0;
        virtual void setPosition( float pos,
                                  float velocity,
                                  std::function< void( Joint ) > callback ) = 0;
        virtual float getTorque() = 0;
        virtual void setTorque( float torque ) = 0;
        virtual void onError(
                std::function< void( Joint, Error, const std::string & ) > callback ) = 0;
    };

    /**
     * \brief Get maximal joint position.
     *
     * \return maximal joint position in rad
     */
    float maxPosition() const
    {
        return _impl->getCapabilities().maxPosition;
    }

    /**
     * \brief Get minimal joint position.
     *
     * \return minimal joint position in rad
     */
    float minPosition() const
    {
        return _impl->getCapabilities().minPosition;
    }

    /**
     * \brief Get maximal joint speed.
     *
     * \return maximal joint speed in rad / s
     */
    float maxSpeed() const
    {
        return _impl->getCapabilities().maxSpeed;
    }

    /**
     * \brief Get minimal joint speed.
     *
     * \return minimal joint speed in rad / s
     */
    float minSpeed() const
    {
        return _impl->getCapabilities().minSpeed;
    }

    /**
     * \brief Get maximal joint torque.
     *
     * \return maximal joint torque in N * m
     */
    float maxTorque() const
    {
        return _impl->getCapabilities().maxTorque;
    }

    /**
     * \brief Get current joint velocity setpoint.
     *
     * \return velocity setpoint in rad / s
     */
    float getVelocity()
    {
        return _impl->getVelocity();
    }

    /**
     * \brief Change the joint's control mode to velocity and move at velocity.
     *
     * \param velocity velocity setpoint in rad / s
     */
    void setVelocity( float velocity )
    {
        _impl->setVelocity( velocity );
    }

    /**
     * \brief Get current joint position.
     *
     * This works in all three control modes - position, velocity and torque mode.
     *
     * \return current position in rad
     */
    float getPosition()
    {
        return _impl->getPosition();
    }

    /**
     * \brief Change the joint's control mode to position and move to position.
     *
     * \param pos position setpoint in rad
     * \param velocity velocity limit in rad / s, required to be positive and non-zero
     * \param callback callback to be called once the position is reached
     */
    void setPosition( float pos, float velocity, std::function< void( Joint ) > callback )
    {
        _impl->setPosition( pos, velocity, callback );
    }

    /**
     * \brief Get current joint torque.
     *
     * \return current torque in N * m
     */
    float getTorque()
    {
        return _impl->getTorque();
    }

    /**
     * \brief Change the joint's control mode to torque and provide torque.
     *
     * \param torque torque setpoint in N * m
     */
    void setTorque( float torque )
    {
        return _impl->setTorque( torque );
    }

    /**
     * \brief Set error handling callback.
     *
     * \param callback callback to be called on error
     */
    void onError( std::function< void( Joint, Error, const std::string & ) > callback )
    {
        _impl->onError( callback );
    }

    Joint( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) )
    {
    }

private:
    std::shared_ptr< Implementation > _impl;
};

/**
 * \brief Connector position.
 */
enum class ConnectorPosition : bool
{
    Retracted = false,
    Extended = true,
};

/**
 * \brief Connection orientation.
 */
enum class ConnectorOrientation : signed char
{
    North = 0,
    East = 1,
    South = 2,
    West = 3,
};

/**
 * \brief Connector power line.
 */
enum class ConnectorLine : bool
{
    Internal = 0,
    External = 1,
};

/**
 * \brief Connector state descriptor.
 */
struct ConnectorState
{
    ConnectorPosition position = ConnectorPosition::Retracted; ///< Position of the Connector.
    bool internal = false;  ///< Is internal power bus connected to the connector?
    bool external = false;  ///< Is the external power bus connected to the connector?
    bool connected = false; ///< Is there a mating side connected?
    ConnectorOrientation orientation = ConnectorOrientation::North;
    ///< Orientation of the connection. Applicable only when connected.
};

/**
 * \brief Connector event.
 */
enum class ConnectorEvent : signed char
{
    Connected = 0,    ///< Connection with other Connector started.
    Disconnected = 1, ///< Connection with other Connector ended.
};

/**
 * \brief Proxy for controlling a single connector of RoFI.
 *
 * Connector cannot be instantiated on its own; you can obtain its instance from
 * RoFI::getConnector().
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class Connector
{
public:
    /**
     * \brief Interface of the actual implementation - either physical local, physical remote or a
     * simulation one.
     *
     * To provide an implementation, simply inherit from this class.
     *
     * For method description, see description of Connector.
     */
    class Implementation : public std::enable_shared_from_this< Implementation >
    {
    protected:
        ~Implementation() = default;

    public:
        virtual ConnectorState getState() = 0;
        virtual void connect() = 0;
        virtual void disconnect() = 0;
        virtual void onConnectorEvent(
                std::function< void( Connector, ConnectorEvent ) > callback ) = 0;
        virtual void onPacket(
                std::function< void( Connector, uint16_t contentType, PBuf ) > callback ) = 0;
        virtual void send( uint16_t contentType, PBuf packet ) = 0;
        virtual void connectPower( ConnectorLine ) = 0;
        virtual void disconnectPower( ConnectorLine ) = 0;
    };

    /**
     * \brief Get connector state.
     *
     * \return connector state
     */
    ConnectorState getState()
    {
        return _impl->getState();
    }

    /**
     * \brief Extend the connector to be ready to accept connection.
     *
     * This action does not establishes connection.
     * To react to connections, register a callback via Connector::onConnectorEvent().
     */
    void connect()
    {
        _impl->connect();
    }

    /**
     * \brief Retract the connector.
     *
     * Does not release a connection immediately.
     * After the connection is broken physically, calls callbacks registered via
     * Connector::onConnectorEvent().
     */
    void disconnect()
    {
        _impl->disconnect();
    }

    /**
     * \brief Register callback for connector events.
     *
     * \param callback callback to be called on connector events
     */
    void onConnectorEvent( std::function< void( Connector, ConnectorEvent ) > callback )
    {
        _impl->onConnectorEvent( callback );
    }

    /**
     * \brief Register callback for incoming packets.
     *
     * The callback takes connector which received the packet, contentType and the PBuf packet.
     *
     * \param callback callback to be called on a packet
     */
    void onPacket( std::function< void( Connector, uint16_t, PBuf ) > callback )
    {
        _impl->onPacket( callback );
    }

    /**
     * \brief Send a packet to mating side.
     *
     * If no mating side is present, packet is discarded.
     */
    void send( uint16_t contentType, PBuf packet )
    {
        _impl->send( contentType, std::move( packet ) );
    }

    /**
     * \brief Connect power of mating side to a power line.
     */
    void connectPower( ConnectorLine line );

    /**
     * \brief Disconnect power of mating side from a power line.
     */
    void disconnectPower( ConnectorLine line );

    Connector( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) )
    {
    }

private:
    std::shared_ptr< Implementation > _impl;
};

/**
 * \brief Proxy for accessing RoFI hardware.
 *
 * The class has a private constructor - you can obtain an instance by calling static methods
 * RoFI::getLocalRoFI and RoFI::getRemoteRoFI.
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class RoFI
{
public:
    /**
     * \brief Unique RoFI identifier.
     */
    using Id = int;

    /**
     * \brief Module shape and capabilities description.
     */
    struct Descriptor
    {
        int jointCount = 0;
        int connectorCount = 0;
    };

    /**
     * \brief Interface of the actual implementation - either physical local, physical remote or a
     * simulation one.
     *
     * To provide an implementation, simply inherit from this class and implement the
     * RoFI::getLocalRoFI() and RoFI::getRemoteRoFI() in a cpp file to return RoFI initialized with
     * an instance of your implementation.
     *
     * For method description, see description of RoFI.
     */
    class Implementation
    {
    protected:
        ~Implementation() = default;

    public:
        virtual Id getId() const = 0;
        virtual Joint getJoint( int index ) = 0;
        virtual Connector getConnector( int index ) = 0;
        virtual Descriptor getDescriptor() const = 0;
    };

    /**
     * \brief Get proxy for controlling local RoFI.
     *
     * \return proxy for controlling local RoFI
     */
    static RoFI getLocalRoFI();

    /**
     *  \brief Get proxy for controlling a remote RoFI identified by Id.
     *
     * \param remoteId Id of the remote RoFI
     *
     * \return proxy for controlling remote RoFI
     */
    static RoFI getRemoteRoFI( Id remoteId );

    /**
     * \brief Get RoFI Id.
     *
     * \return RoFI Id
     */
    Id getId() const
    {
        return _impl->getId();
    }

    /**
     * \brief Get proxy for controlling Joint.
     *
     * \param index index of the Joint
     *
     * \return proxy for controlling Joint
     */
    Joint getJoint( int index )
    {
        return _impl->getJoint( index );
    }

    /**
     * \brief Get proxy for controlling Connector.
     *
     * \param index index of the Connector
     *
     * \return proxy for controlling Connector
     */
    Connector getConnector( int index )
    {
        return _impl->getConnector( index );
    }

    /**
     * \brief Get RoFI Descriptor.
     *
     * \return RoFI Descriptor
     */
    Descriptor getDescriptor() const
    {
        return _impl->getDescriptor();
    }

    /**
     * \brief Call callback after given delay.
     *
     * \param ms delay in milliseconds
     * \param callback callback to be called after the delay
     */
    static void wait( int ms, std::function< void() > callback );

private:
    RoFI( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) )
    {
    }
    std::shared_ptr< Implementation > _impl;
};

} // namespace rofi::hal
