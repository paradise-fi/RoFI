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
            boost::shared_ptr< Data > rdata;

            RoFI();

        public:
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
                float maxPosition() const;
                float minPosition() const;
                float maxSpeed() const;
                float minSpeed() const;
                float maxTorque() const;
                float getVelocity() const;
                void setVelocity( float velocity );
                float getPosition() const;
                void setPosition( float pos, float speed, void ( *callback )( Joint ) );
                float getTorque() const;
                void setTorque( float torque );
                // TODO void onError( void ( *callback )( JointError ) );

            };
        };
    }
}
