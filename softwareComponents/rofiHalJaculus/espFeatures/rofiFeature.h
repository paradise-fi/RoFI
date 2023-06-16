#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/class.h>

#include <noal_func.h>
#include <memory>
#include <unordered_map>

#include <rofi_hal.hpp>
#include <atoms/units.hpp>
#include <cstdlib>

#include <set>

template<class Next>
class RofiFeature : public Next {
    // static inline std::set<int> _usedRmtChannels;

    struct RofiJointProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::Joint>, public jac::ProtoBuilder::Properties {
        static void addProperties(JSContext* ctx, jac::Object proto) {
            const int speedCoef = 1;
            jac::FunctionFactory ff(ctx);

            proto.defineProperty("setVelocity", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, double velocity) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                joint.setVelocity(velocity);
            }), jac::PropFlags::Enumerable);


            proto.defineProperty("getVelocity", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.getVelocity();
            }), jac::PropFlags::Enumerable);

            proto.defineProperty("setPosition", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, double position_raw) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);

                auto position = Angle::deg( position_raw );
                float speed = speedCoef * joint.maxSpeed();
                if ( position.rad() > joint.maxPosition() || position.rad() < joint.minPosition() )
                    throw std::runtime_error( "The requested position is out of range" );

                joint.setPosition( position.rad(), speed, [] ( auto ){} );
            }), jac::PropFlags::Enumerable);

            proto.defineProperty("move", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                joint.setPosition( -1.5, 1.5, []( rofi::hal::Joint ){ std::cout << "\tDone1\n"; } );
                vTaskDelay( 2000 / portTICK_PERIOD_MS );
                joint.setPosition( 1.5, 1.5, []( rofi::hal::Joint ){ std::cout << "\tDone2\n"; } );
                vTaskDelay( 2000 / portTICK_PERIOD_MS );
            }), jac::PropFlags::Enumerable);



        }
    };

    struct RofiConnectorProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::Connector>, public jac::ProtoBuilder::Properties {

        static void addProperties(JSContext* ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            proto.defineProperty("connect", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                connector.connect();
            }), jac::PropFlags::Enumerable);


            proto.defineProperty("disconnect", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                return connector.disconnect();
            }), jac::PropFlags::Enumerable);
        }
    };

    struct RofiProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::RoFI>, public jac::ProtoBuilder::Properties {
        static void addProperties(JSContext* ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            proto.defineProperty("getId", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                return rofi.getId();
            }), jac::PropFlags::Enumerable);

            proto.defineProperty("getRandomNumber", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                return rand() % 100;
            }), jac::PropFlags::Enumerable);


            proto.defineProperty("getConnector", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, int index) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                auto *connectorPointer = new rofi::hal::Connector(rofi.getConnector(index));
                return RofiConnectorClass::createInstance(ctx, connectorPointer);
            }), jac::PropFlags::Enumerable);

            proto.defineProperty("getJoint", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, int index) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                auto *jointPointer = new rofi::hal::Joint(rofi.getJoint(index));
                return RofiJointClass::createInstance(ctx, jointPointer);
            }), jac::PropFlags::Enumerable);
        }
    };


public:
    using RofiJointClass = jac::Class<RofiJointProtoBuilder>;
    using RofiConnectorClass = jac::Class<RofiConnectorProtoBuilder>;
    using RofiClass = jac::Class<RofiProtoBuilder>;

    RofiFeature() {
        RofiJointClass::init("Joint");
        RofiConnectorClass::init("Connector");
        RofiClass::init("RoFI");
    }

    void initialize() {
        Next::initialize();
        // jac::ContextRef ctx = this->context();

        RofiJointClass::initContext(this->context());
        RofiConnectorClass::initContext(this->context());
        RofiClass::initContext(this->context());

        jac::FunctionFactory ff(this->context());

        auto& mod = this->newModule("rofi");
        mod.addExport("getLocalRofi", ff.newFunction([this]() {
            // auto *rofiPointer = new rofi::hal::RoFI(std::move(rofi::hal::RoFI::getLocalRoFI()));
            auto *rofiPointer = new rofi::hal::RoFI(rofi::hal::RoFI::getLocalRoFI());
            // auto *rofiPointer = new rofi::hal::RoFI();
            return RofiClass::createInstance(this->context(), rofiPointer);
        }));
    }
};
