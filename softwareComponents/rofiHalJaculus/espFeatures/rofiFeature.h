#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/class.h>

#include <rofi_hal.hpp>
#include "rofiConvTraits.h"

template<class Next>
class RofiFeature : public Next {
    struct RofiJointProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::Joint>, public jac::ProtoBuilder::Properties {
        static void addProperties(JSContext* ctx, jac::Object proto) {
            const int speedCoef = 1;
            jac::FunctionFactory ff(ctx);

            // maximal joint position in rad
            proto.defineProperty("maxPosition", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.maxPosition();
            }), jac::PropFlags::Enumerable);

            // minimal joint position in rad
            proto.defineProperty("minPosition", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.minPosition();
            }), jac::PropFlags::Enumerable);

            //maximal joint speed in rad / s
            proto.defineProperty("maxSpeed", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.maxSpeed();
            }), jac::PropFlags::Enumerable);

            // minimal joint speed in rad / s
            proto.defineProperty("minSpeed", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.minSpeed();
            }), jac::PropFlags::Enumerable);

            // maximal joint torque in N * m
            proto.defineProperty("maxTorque", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.maxTorque();
            }), jac::PropFlags::Enumerable);


            // velocity setpoint in rad / s
            proto.defineProperty("getVelocity", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.getVelocity();
            }), jac::PropFlags::Enumerable);

            // velocity setpoint in rad / s
            proto.defineProperty("setVelocity", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, double velocity) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                joint.setVelocity(velocity);
            }), jac::PropFlags::Enumerable);


            // current position in rad
            proto.defineProperty("getPosition", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.getPosition();
            }), jac::PropFlags::Enumerable);

            /**
            * \param pos position setpoint in rad
            * \param velocity velocity limit in rad / s, required to be positive and non-zero
            * \param callback callback to be called once the position is reached*/
            proto.defineProperty("setPosition", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, float pos, float velocity, jac::Function callback) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                if ( pos > joint.maxPosition() || pos < joint.minPosition() )
                    throw std::runtime_error( "The requested position is out of range" );
                joint.setPosition( pos, velocity, [] ( auto ){} );
            }), jac::PropFlags::Enumerable);


            // current torque in N * m
            proto.defineProperty("getTorque", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.getTorque();
            }), jac::PropFlags::Enumerable);

            // torque setpoint in N * m
            proto.defineProperty("setTorque", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, float torque) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                joint.setTorque(torque);
            }), jac::PropFlags::Enumerable);


            // callback to be called on error
            proto.defineProperty("onError", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, jac::Function callback) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                throw std::runtime_error( "Not implemented" );
            }), jac::PropFlags::Enumerable);

            // test servo move
            proto.defineProperty("testMove", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
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

            // connector state
            proto.defineProperty("getState", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                throw std::runtime_error("Not implemented");
                // return connector.getState();
            }), jac::PropFlags::Enumerable);

            // Extend the connector to be ready to accept connection.
            proto.defineProperty("connect", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                connector.connect();
            }), jac::PropFlags::Enumerable);

            // Retract the connector.
            proto.defineProperty("disconnect", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                return connector.disconnect();
            }), jac::PropFlags::Enumerable);

            // callback to be called on connector events
            proto.defineProperty("onEvent", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, jac::Function callback) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                throw std::runtime_error("Not implemented");
            }), jac::PropFlags::Enumerable);

            // // callback to be called on a packet
            // proto.defineProperty("onPacket", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, jac::Function callback) {
            //     rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
            //     throw std::runtime_error("Not implemented");
            // }), jac::PropFlags::Enumerable);

            // Connect power of mating side to a power line.
            proto.defineProperty("connectPower", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, rofi::hal::ConnectorLine line) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                connector.connectPower(line);
            }), jac::PropFlags::Enumerable);

            // power line to disconnect from
            proto.defineProperty("disconnectPower", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, rofi::hal::ConnectorLine line) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                connector.disconnectPower(line);
            }), jac::PropFlags::Enumerable);

            // Sets distance mode for the RoFICoM's Lidar.
            proto.defineProperty("setDistanceMode", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, rofi::hal::LidarDistanceMode mode) {
                rofi::hal::Connector& connector = *getOpaque(ctx, thisValue);
                connector.setDistanceMode(mode);
            }), jac::PropFlags::Enumerable);
        }
    };

    struct RofiProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::RoFI>, public jac::ProtoBuilder::Properties {
        static void addProperties(JSContext* ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            // RoFI Id
            proto.defineProperty("getId", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                return rofi.getId();
            }), jac::PropFlags::Enumerable);

            proto.defineProperty("getRandomNumber", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                return rand() % 100;
            }), jac::PropFlags::Enumerable);

            // proxy for controlling Joint
            proto.defineProperty("getJoint", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, int index) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                auto *jointPointer = new rofi::hal::Joint(rofi.getJoint(index));
                return RofiJointClass::createInstance(ctx, jointPointer);
            }), jac::PropFlags::Enumerable);

            // proxy for controlling Connector
            proto.defineProperty("getConnector", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, int index) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                auto *connectorPointer = new rofi::hal::Connector(rofi.getConnector(index));
                return RofiConnectorClass::createInstance(ctx, connectorPointer);
            }), jac::PropFlags::Enumerable);

            // RoFI Descriptor
            proto.defineProperty("getDescriptor", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                return rofi.getDescriptor();
            }), jac::PropFlags::Enumerable);

            // Reboot device
            proto.defineProperty("reboot", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                rofi.reboot();
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
        jac::ContextRef ctx = this->context();

        RofiJointClass::initContext(ctx);
        RofiConnectorClass::initContext(ctx);
        RofiClass::initContext(ctx);

        jac::FunctionFactory ff(ctx);

        auto& mod = this->newModule("rofi");
        mod.addExport("getLocalRofi", ff.newFunction([ctx]() {
            auto *rofiPointer = new rofi::hal::RoFI(rofi::hal::RoFI::getLocalRoFI());
            return RofiClass::createInstance(ctx, rofiPointer);
        }));
    }
};
