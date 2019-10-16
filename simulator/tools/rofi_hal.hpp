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
            std::shared_ptr< Data > rdata;

        public:
            // TODO constructors
            explicit RoFI();

            Joint getJoint( int index );

            class Joint
            {
            public:
                class Data;
                // TODO class JointError {};
                std::shared_ptr< Data > jdata;

            private:
                friend class RoFI::Data;
                Joint() = default;

            public:
                float maxPosition() const;
                float minPosition() const;
                float maxSpeed() const;
                float minSpeed() const;
                float maxTorque() const;
                float getSpeed() const;
                void setSpeed( float speed );
                float getPosition() const;
                void setPosition( float pos, float speed, void (*callback)( Joint ));
                float getTorque() const;
                void setTorque( float torque );
                // TODO void onError( void ( *callback )( JointError ) );

            };
        };
    }
}
