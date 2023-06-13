#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/class.h>

#include <noal_func.h>
#include <memory>
#include <unordered_map>
#include "../rofi/rofi_hal.hpp"

#include <set>

template<class Next>
class RofiFeature : public Next {
    // static inline std::set<int> _usedRmtChannels;

    struct RofiJointProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::Joint>, public jac::ProtoBuilder::Properties {
        static rofi::hal::Joint* constructOpaque(JSContext* ctx, std::vector<jac::ValueWeak> args) {
            return new rofi::hal::Joint();
        }

        static void destroyOpaque(JSRuntime* rt, rofi::hal::Joint* ptr) noexcept {
            if (!ptr) return;
            delete ptr;
        }

        static void addProperties(JSContext* ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            proto.defineProperty("setVelocity", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue, double velocity) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                joint.setVelocity(velocity);
            }), jac::PropFlags::Enumerable);


            proto.defineProperty("getVelocity", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::Joint& joint = *getOpaque(ctx, thisValue);
                return joint.getVelocity();
            }), jac::PropFlags::Enumerable);
        }
    };

    struct RofiConnectorProtoBuilder : public jac::ProtoBuilder::Opaque<rofi::hal::Connector>, public jac::ProtoBuilder::Properties {
        static rofi::hal::Connector* constructOpaque(JSContext* ctx, std::vector<jac::ValueWeak> args) {
            return new rofi::hal::Connector();
        }

        static void destroyOpaque(JSRuntime* rt, rofi::hal::Connector* ptr) noexcept {
            if (!ptr) return;
            delete ptr;
        }

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
        static rofi::hal::RoFI* constructOpaque(JSContext* ctx, std::vector<jac::ValueWeak> args) {
            return new rofi::hal::RoFI();
        }

        static void destroyOpaque(JSRuntime* rt, rofi::hal::RoFI* ptr) noexcept {
            if (!ptr) return;
            delete ptr;
        }

        static void addProperties(JSContext* ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            proto.defineProperty("getId", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                return rofi.getId();
            }), jac::PropFlags::Enumerable);


            proto.defineProperty("getConnector", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
                // rofi::hal::RoFI& rofi = *getOpaque(ctx, thisValue);
                // rofi::hal::Connector& connector = rofi.getConnector(index);
                auto connector = RofiConnectorProtoBuilder::getOpaque(ctx, thisValue);

                jac::Value::to<jac::ObjectWeak>(ctx, connector);
                return connector;
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

        auto& mod = this->newModule("rofi");
        jac::Function joint = RofiJointClass::getConstructor(this->context());
        mod.addExport("Joint", joint);

        jac::Function connector = RofiConnectorClass::getConstructor(this->context());
        mod.addExport("Connector", connector);

        jac::Function rofi = RofiClass::getConstructor(this->context());
        mod.addExport("RoFI", rofi);
    }
};
