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
            friend class detail::ConnectorData;

            detail::ConnectorData * connectorData;

            Connector( detail::ConnectorData & cdata );

        public:
            Connector( const Connector & ) = default;
            Connector & operator=( const Connector & ) = default;

            // State getState() const;
            void connect();
            void disconnect();
            // void onConnectorEvent( std::function< void ( Connector, ConnectorEvent ) > callback );
            // void onPacket( std::function< void ( Connector, Packet ) > callback );
            // void send( Packet packet );
            // void connectPower( Line );
            // void disconnectPower( Line );
            // PowerState getPowerState( Line );
        };
    } // namespace hal
} // namespace rofi
