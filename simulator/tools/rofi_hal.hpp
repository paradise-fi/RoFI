#include <functional>
#include <memory>

namespace rofi
{
    namespace hal
    {
        class RoFI
        {
        public:
            class Data;
            class Joint;

        private:
            std::unique_ptr< Data > rdata;

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

            class Joint
            {
            public:
                class Data;
                // TODO class JointError {};

                Data * jdata;

            private:
                friend class RoFI::Data;
                Joint( Data & data );

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
        };
    }
}
