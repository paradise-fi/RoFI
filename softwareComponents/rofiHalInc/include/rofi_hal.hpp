#pragma once
#include <functional>
#include <memory>
#include <span>

#include "networking.hpp"

namespace rofi::hal {

/**
 * \brief Proxy for controlling a single joint of RoFI.
 *
 * Joint cannot be instantiated on its own; you can obtain its instance from
 * RoFI::getJoint().
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class Joint {
public:
    /**
     * \brief
     */
    enum class Error {
        Communication, ///< Cannot communicate with the servo motor.
        Hardware,      ///< Servo motor responded with an error.
    };

    /**
     * \brief Interface of the actual implementation - either physical local,
     * physical remote or a simulation one.
     *
     * To provide an implementation, simply inherit from this class.
     *
     * For method description, see description of Joint.
     */
    class Implementation {
    protected:
        ~Implementation() = default;

    public:
        struct Capabilities {
            float maxPosition, minPosition;
            float maxSpeed, minSpeed;
            float maxTorque;
        };

        virtual const Capabilities& getCapabilities() = 0;
        virtual float getVelocity() = 0;
        virtual void setVelocity( float velocity ) = 0;
        virtual float getPosition() = 0;
        virtual void setPosition( float pos,
                                  float velocity,
                                  std::function< void( Joint ) > callback ) = 0;
        virtual float getTorque() = 0;
        virtual void setTorque( float torque ) = 0;
        virtual void onError(
            std::function< void( Joint, Error, const std::string& ) > callback ) = 0;
    };

    /**
     * \brief Get maximal joint position.
     *
     * \return maximal joint position in rad
     */
    float maxPosition() const { return _impl->getCapabilities().maxPosition; }

    /**
     * \brief Get minimal joint position.
     *
     * \return minimal joint position in rad
     */
    float minPosition() const { return _impl->getCapabilities().minPosition; }

    /**
     * \brief Get maximal joint speed.
     *
     * \return maximal joint speed in rad / s
     */
    float maxSpeed() const { return _impl->getCapabilities().maxSpeed; }

    /**
     * \brief Get minimal joint speed.
     *
     * \return minimal joint speed in rad / s
     */
    float minSpeed() const { return _impl->getCapabilities().minSpeed; }

    /**
     * \brief Get maximal joint torque.
     *
     * \return maximal joint torque in N * m
     */
    float maxTorque() const { return _impl->getCapabilities().maxTorque; }

    /**
     * \brief Get current joint velocity setpoint.
     *
     * \return velocity setpoint in rad / s
     */
    float getVelocity() { return _impl->getVelocity(); }

    /**
     * \brief Change the joint's control mode to velocity and move at velocity.
     *
     * \param velocity velocity setpoint in rad / s
     */
    void setVelocity( float velocity ) { _impl->setVelocity( velocity ); }

    /**
     * \brief Get current joint position.
     *
     * This works in all three control modes - position, velocity and torque mode.
     *
     * \return current position in rad
     */
    float getPosition() { return _impl->getPosition(); }

    /**
     * \brief Change the joint's control mode to position and move to position.
     *
     * Calling any joint control method  (`Joint::setPosition`,
     * `Joint::setVelocity` or `Joint::setTorque`) before the position is
     * reached will result in the \p callback never beeing called.
     * \param pos position setpoint in rad
     * \param velocity velocity limit in rad / s, required to be positive and non-zero
     * \param callback callback to be called once the position is reached
     */
    void setPosition( float pos, float velocity, std::function< void( Joint ) > callback )
    {
        _impl->setPosition( pos, velocity, std::move( callback ) );
    }

    /**
     * \brief Get current joint torque.
     *
     * \return current torque in N * m
     */
    float getTorque() { return _impl->getTorque(); }

    /**
     * \brief Change the joint's control mode to torque and provide torque.
     *
     * \param torque torque setpoint in N * m
     */
    void setTorque( float torque ) { return _impl->setTorque( torque ); }

    /**
     * \brief Set error handling callback.
     *
     * Calling this method again (on the same `Joint`) will overwrite
     * the previous `callback`.
     * You can call this method with empty function to remove the `callback`.
     * \param callback callback to be called on error
     */
    void onError( std::function< void( Joint, Error, const std::string& ) > callback )
    {
        _impl->onError( std::move( callback ) );
    }

    Joint( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) ) {}

private:
    std::shared_ptr< Implementation > _impl;
};

/**
 * \brief Connector position.
 */
enum class ConnectorPosition : bool {
    Retracted = false,
    Extended = true,
};

/**
 * \brief Connection orientation.
 */
enum class ConnectorOrientation : signed char {
    North = 0,
    East = 1,
    South = 2,
    West = 3,
};

/**
 * \brief Connector power line.
 */
enum class ConnectorLine : bool {
    Internal = 0,
    External = 1,
};

enum class LidarDistanceMode {
    /// The `Autonomous` Distance Mode is handled by RoFICoM based on the status of measurements.
    Autonomous = 0,
    /// `Short` mode measures up to 1.3 m and is less effected by the ambient light.
    Short      = 1,
    /// `Long` mode can measure up to 4 m; however is strongly effected by the ambient light.
    /// The `LidarStatus` can reflect this mode might not be suitable for current condition by the `OutsideRange` value.
    Long       = 2,
};

/**
 * \brief Status of Lidar measurement
*/
enum class LidarStatus : signed char {
    /// Usually error with lidar communication i.e. Lidar not connected, i2c error, ...
    Error = 0b00,
    /// Status command was received before lidar initilized and received measurements.
    NotMeasured = 0b01,
    /// Measured data are outside of lidar available range 
    /// meaning that data could be valid but DOESN'T HAVE TO BE.
    /// Usually means that measured data is below or above of range we can measure.
    /// Also could be caused by wrong distance mode, setting different one could resolve this.
    OutsideRange = 0b10,
    /// Measured data are fully valid
    Valid = 0b11,
};

/**
 * \brief Connector state descriptor.
 */
struct ConnectorState {
    /// Position of the Connector.
    ConnectorPosition position = ConnectorPosition::Retracted;
    /// Is internal power bus connected to the connector?
    bool internal = false;
    /// Is the external power bus connected to the connector?
    bool external = false;
    /// The current distance mode of the RoFICoM's Lidar.
    LidarDistanceMode distanceMode = LidarDistanceMode::Autonomous;
    /// Is there a mating side connected?
    bool connected = false;
    /// Orientation of the connection. Applicable only when connected.
    ConnectorOrientation orientation = ConnectorOrientation::North;
    /// Internal voltage of the Connector.
    float internalVoltage = 0.f;
    /// Internal current of the Connector.
    float internalCurrent = 0.f;
    /// External voltage of the Connector.
    float externalVoltage = 0.f;
    /// External current of the Connector.
    float externalCurrent = 0.f;
    /// Status of lidar measurement.
    LidarStatus lidarStatus = LidarStatus::NotMeasured;
    /// Lidar measured distance in mm. Validity depends on `lidarStatus`.
    uint16_t distance = 0;
};

/**
 * \brief Connector event.
 */
enum class ConnectorEvent : signed char {
    /// Connection with other Connector started.
    Connected,
    /// Connection with other Connector ended.
    Disconnected,
    /// Power status (voltage or current) changed.
    PowerChanged,
};

/**
 * \brief Proxy for accessing raw partition data
 *
 * Partition cannot be instantiated on its own; you can obtain its instance from
 * RoFI::getRunningPartition().
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class Partition {
    public:
        /**
         * \brief Interface of the actual implementation - either physical or simulation one.
         *
         * To provide an implementation, simply inherit from this class.
         *
         * For method description, see description of Partition.
         */
        class Implementation {
        protected:
            virtual ~Implementation() = default;

        public:
            virtual std::size_t getSize() = 0;
            virtual void read( std::size_t offset, std::size_t size, const std::span< unsigned char >& buffer ) = 0;
            virtual void write( std::size_t offset, const std::span< const unsigned char >& data ) = 0;
        };

        /**
         * \brief Get partition size.
         *
         * \return partition size in bytes
         */
        std::size_t getSize() const { return _impl->getSize(); }

        /**
         * \brief Read exactly `size` bytes into `buffer` from the partition starting at the `offset`.
         *
         * If read would go out of bounds of the partition `std::out_of_range` is thrown.
         *
         * \param offset offset from beginning of the partition to start reading from
         * \param size number of bytes to read
         * \param buffer buffer to put read data to
         *
         * \throw std::out_of_range if read would go out of bounds of the partition
         */
        void read( std::size_t offset, std::size_t size, const std::span< unsigned char >& buffer ) {
            _impl->read( offset, size, buffer );
        }

        /**
         * \brief Write `data` to the partition starting at the `offset`.
         *
         * If write would go out of bounds of the partition `std::out_of_range` is thrown.
         *
         * \param offset offset from beginning of the partition to start writing at
         * \param data data to write to the partition
         *
         * \throw std::out_of_range if write would go out of bounds of the partition
         */
        void write( std::size_t offset, const std::span< const unsigned char >& data ) {
            _impl->write( offset, data );
        }

        Partition( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) ) {}

    protected:
        std::shared_ptr< Implementation > _impl;
    };

/**
 * \brief Proxy for accessing update partition data
 *
 * UpdatePartition cannot be instantiated on its own; you can obtain its instance from
 * RoFI::initUpdate().
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
    class UpdatePartition : public Partition {
    public:
        /**
         * \brief Interface of the actual implementation - either physical or simulation one.
         *
         * To provide an implementation, simply inherit from this class.
         *
         * For method description, see description of UpdatePartition.
         */
        class Implementation : virtual public Partition::Implementation {
        protected:
            virtual ~Implementation() override = default;

        public:
            virtual void commit() = 0;
            virtual void abort() = 0;
        };

        /**
         * \brief Commit pending update
         *
         * By calling this method update process is ended hence subsequent calls of this method, UpdatePartition::abort
         * or UpdatePartition::write method may throw std::runtime_error.
         * If any error occurs during update verification std::runtime_error is thrown with particular error reason.
         */
        void commit() {
            dynamic_cast< UpdatePartition::Implementation* >( _impl.get() )->commit();
        }

        /**
         * \brief Abort pending update
         *
         * By calling this method update process is ended hence subsequent calls of this method, UpdatePartition::commit
         * or UpdatePartition::write method may throw std::runtime_error.
         */
        void abort() {
            dynamic_cast< UpdatePartition::Implementation* >( _impl.get() )->abort();
        }

        UpdatePartition( std::shared_ptr< Implementation > impl ) : Partition( std::move( impl ) ) {}
    };

/**
 * \brief Proxy for controlling a single connector of RoFI.
 *
 * Connector cannot be instantiated on its own; you can obtain its instance from
 * RoFI::getConnector().
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class Connector {
public:
    /**
     * \brief Interface of the actual implementation - either physical local,
     * physical remote or a simulation one.
     *
     * To provide an implementation, simply inherit from this class.
     *
     * For method description, see description of Connector.
     */
    class Implementation {
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
        virtual void setDistanceMode( LidarDistanceMode ) = 0;
    };

    /**
     * \brief Get connector state.
     *
     * \return connector state
     */
    ConnectorState getState() { return _impl->getState(); }

    /**
     * \brief Extend the connector to be ready to accept connection.
     *
     * This action does not establish connection.
     * To react to connections, register a callback via Connector::onConnectorEvent().
     */
    void connect() { _impl->connect(); }

    /**
     * \brief Retract the connector.
     *
     * Does not release a connection immediately.
     * After the connection is broken physically, calls callback registered via
     * Connector::onConnectorEvent().
     */
    void disconnect() { _impl->disconnect(); }

    /**
     * \brief Register callback for connector events.
     *
     * Calling this method again (on the same `Connector`) will overwrite
     * the previous `callback`.
     * You can call this method with empty function to remove the `callback`.
     * \param callback callback to be called on connector events
     */
    void onConnectorEvent( std::function< void( Connector, ConnectorEvent ) > callback )
    {
        _impl->onConnectorEvent( std::move( callback ) );
    }

    /**
     * \brief Register callback for incoming packets.
     *
     * The callback takes connector which received the packet, contentType and the packet.
     *
     * Calling this method again (on the same `Connector`) will overwrite
     * the previous `callback`.
     * You can call this method with empty function to remove the `callback`.
     * \param callback callback to be called on a packet
     */
    void onPacket( std::function< void( Connector, uint16_t, PBuf ) > callback )
    {
        _impl->onPacket( std::move( callback ) );
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
    void connectPower( ConnectorLine line )
    {
        _impl->connectPower( line );
    }

    /**
     * \brief Disconnect power of mating side from a power line.
     */
    void disconnectPower( ConnectorLine line )
    {
        _impl->disconnectPower( line );
    }

    /**
     * \brief Sets distance mode for the RoFICoM's Lidar.
    */
    void setDistanceMode( LidarDistanceMode mode )
    {
        _impl->setDistanceMode( mode );
    }

    Connector( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) ) {}

private:
    std::shared_ptr< Implementation > _impl;
};

/**
 * \brief Proxy for accessing RoFI hardware.
 *
 * The class has a private constructor - you can obtain an instance by calling
 * static methods RoFI::getLocalRoFI and RoFI::getRemoteRoFI.
 *
 * Since the class is only a proxy it can be safely copied and passed around.
 */
class RoFI {
public:
    /**
     * \brief Unique RoFI identifier.
     */
    using Id = int;

    /**
     * \brief Module shape and capabilities description.
     */
    struct Descriptor {
        int jointCount = 0;
        int connectorCount = 0;
    };

    /**
     * \brief Interface of the actual implementation - either physical local,
     * physical remote or a simulation one.
     *
     * To provide an implementation, simply inherit from this class and
     * implement the RoFI::getLocalRoFI() and RoFI::getRemoteRoFI() in a cpp
     * file to return RoFI initialized with an instance of your implementation.
     *
     * For method description, see description of RoFI.
     */
    class Implementation {
    protected:
        ~Implementation() = default;

    public:
        virtual Id getId() const = 0;
        virtual Joint getJoint( int index ) = 0;
        virtual Connector getConnector( int index ) = 0;
        virtual Descriptor getDescriptor() const = 0;
        virtual Partition getRunningPartition() = 0;
        virtual UpdatePartition initUpdate() = 0;
        virtual void reboot() = 0;
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
    Id getId() const { return _impl->getId(); }

    /**
     * \brief Get proxy for controlling Joint.
     *
     * \param index index of the Joint
     *
     * \return proxy for controlling Joint
     */
    Joint getJoint( int index ) { return _impl->getJoint( index ); }

    /**
     * \brief Get proxy for controlling Connector.
     *
     * \param index index of the Connector
     *
     * \return proxy for controlling Connector
     */
    Connector getConnector( int index ) { return _impl->getConnector( index ); }

    /**
     * \brief Get RoFI Descriptor.
     *
     * \return RoFI Descriptor
     */
    Descriptor getDescriptor() const { return _impl->getDescriptor(); }

    /**
 * \brief Get partition from which running firmware was loaded
 *
 * \return partition from which running firmware was loaded
 */
    Partition getRunningPartition() const { return _impl->getRunningPartition(); }

    /**
     * \brief Initialize firmware update
     *
     * If this method is called and another update is already in process std::runtime_error is thrown
     *
     * @return UpdatePartition to write the new firmware to
     */
    UpdatePartition initUpdate() { return _impl->initUpdate(); }

    /**
     * \brief Reboot device
     */
    void reboot() { return _impl->reboot(); }

    /**
     * \brief Call callback after given delay. The call is blocking.
     *
     * You can assume that the callback will always be called exactly once.
     * \param ms delay in milliseconds
     * \param callback non-empty callback to be called after the delay
     */
    static void wait( int ms, std::function< void() > callback );

    /**
     * \brief Call callback after given delay. The call is non-blocking.
     *
     * You can assume that the callback will always be called exactly once.
     * \param ms delay in milliseconds
     * \param callback non-empty callback to be called after the delay
     */
    static void delay( int ms, std::function< void() > callback );

private:
    RoFI( std::shared_ptr< Implementation > impl ) : _impl( std::move( impl ) ) {}
    std::shared_ptr< Implementation > _impl;
};

} // namespace rofi::hal
