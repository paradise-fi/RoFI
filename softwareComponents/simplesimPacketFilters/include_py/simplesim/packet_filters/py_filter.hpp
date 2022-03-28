#pragma once

#include <memory>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <simplesim/packet_filter.hpp>


namespace rofi::simplesim::packetf
{
namespace py = pybind11;

/**
 * \brief Creates a python interpreter guard that can be used in multiple
 * places at once
 *
 * Python interpreter requires only one instance to be present
 * (multiple calls to init functions is an error).
 * This class deals with using python interpreter in multiple places without
 * worrying about other uses.
 * Also it allows the destruction of the interpreter when it is no longer
 * needed (by using a weak reference to the instance).
 */
class [[nodiscard]] GlobalPyInterpreterGuard {
private:
    static std::weak_ptr< py::scoped_interpreter > globalInterpreter;

public:
    // Not thread safe
    static auto instance() -> GlobalPyInterpreterGuard
    {
        auto interpreter = globalInterpreter.lock();
        if ( !interpreter ) {
            interpreter = std::make_shared< py::scoped_interpreter >();
            globalInterpreter = interpreter;
        }
        assert( interpreter );
        return GlobalPyInterpreterGuard( std::move( interpreter ) );
    }

private:
    GlobalPyInterpreterGuard( std::shared_ptr< py::scoped_interpreter > interpreter )
            : _interpreter( std::move( interpreter ) )
    {}

    std::shared_ptr< py::scoped_interpreter > _interpreter;
};


class PyFilter {
public:
    PyFilter( const std::string & filterCode );

    auto filter( PacketFilter::SendPacketData packetData ) -> PacketFilter::DelayedPacket;
    auto operator()( auto && packetData ) -> PacketFilter::DelayedPacket
    {
        return filter( std::forward< decltype( packetData ) >( packetData ) );
    }

private:
    GlobalPyInterpreterGuard _pyGuard;
    py::object _scope;
};

} // namespace rofi::simplesim::packetf
