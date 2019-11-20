#include <cstddef>
#include <functional>
#include <memory>

namespace rofi
{
    namespace hal
    {
        namespace detail
        {
            class RoFIData;
            class JointData;
            class ConnectorData;
        } // namespace detail

        class RoFI;
        class Joint;
        class Connector;

        enum class ConnectorPosition : bool;
        enum class ConnectorOrientation : signed char;
        enum class ConnectorLine : bool;
        struct ConnectorState;

        class RoFI
        {
            std::unique_ptr< detail::RoFIData > rofiData;

            RoFI();

        public:
            // Destructor is default, but has to be defined in implementation (for unique_ptr)
            ~RoFI();

            RoFI( const RoFI & ) = delete;
            RoFI & operator=( const RoFI & ) = delete;

            RoFI( RoFI && ) = default;
            RoFI & operator=( RoFI && ) = default;

            static RoFI & getLocalRoFI();

            Joint getJoint( int index );
            Connector getConnector( int index );
        };

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
            void setPosition( float pos, float speed, std::function< void ( Joint ) > callback );
            float getTorque() const;
            void setTorque( float torque );
            // TODO void onError( void ( *callback )( JointError ) );
        };

        class Connector
        {
        public:
            using State = ConnectorState;
            using Position = ConnectorPosition;
            using Orientation = ConnectorOrientation;

            using Packet = std::vector< std::byte >;

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
            // TODO void onConnectorEvent( std::function< void ( Connector, ConnectorEvent ) > callback );
            void onPacket( std::function< void ( Connector, Packet ) > callback );
            void send( Packet packet );
            void connectPower( ConnectorLine );
            void disconnectPower( ConnectorLine );
        };

        enum class ConnectorPosition : bool
        {
            Retracted = false,
            Expanded = true
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
    } // namespace hal
} // namespace rofi
