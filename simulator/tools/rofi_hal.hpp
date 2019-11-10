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
        } // namespace detail

        class RoFI;
        class Joint;

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
    } // namespace hal
} // namespace rofi
