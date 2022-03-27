#include "simplesim/packet_filters/py_filter.hpp"

#include <stdexcept>
#include <utility>

#include <pybind11/eval.h>
#include <pybind11/numpy.h>


namespace rofi::simplesim::packetf
{

std::weak_ptr< py::scoped_interpreter > GlobalPyInterpreterGuard::globalInterpreter = {};


PYBIND11_EMBEDDED_MODULE( rofipacketf, m )
{
    m.doc() = "RoFI simplesim packet filter";

    py::class_< rofi::messages::Packet >( m, "Packet", py::buffer_protocol() )
            .def( py::init( []( py::array_t< char > packedData ) {
                if ( packedData.ndim() != 1 ) {
                    throw std::runtime_error( "Incompatible buffer dimension!" );
                }

                auto packet = rofi::messages::Packet();
                packet.set_message( packedData.data(), packedData.size() );
                return packet;
            } ) )
            .def_buffer( []( const rofi::messages::Packet & self ) -> py::buffer_info {
                return py::buffer_info( self.message().data(), self.message().size() );
            } );

    py::class_< Connector >( m, "Connector" )
            .def_readwrite( "module_id", &Connector::moduleId )
            .def_readwrite( "connector_idx", &Connector::connIdx )
            .def( "__repr__", []( const Connector & self ) {
                return "<Connector " + std::to_string( self.connIdx ) + " of Module with Id "
                     + std::to_string( self.moduleId ) + ">";
            } );

    py::class_< PacketFilter::DelayedPacket >( m, "DelayedPacket" )
            .def_readwrite( "packet", &PacketFilter::DelayedPacket::packet )
            .def_property(
                    "delay_ms",
                    []( const PacketFilter::DelayedPacket & self ) { return self.delay.count(); },
                    []( PacketFilter::DelayedPacket & self, int64_t delayMs ) {
                        self.delay = std::chrono::milliseconds( delayMs );
                    } )
            .def( py::init( []( rofi::messages::Packet packet, int64_t delayMs ) {
                      return PacketFilter::DelayedPacket{ .packet = std::move( packet ),
                                                          .delay = std::chrono::milliseconds(
                                                                  delayMs ) };
                  } ),
                  py::arg( "packet" ),
                  py::arg( "delay_ms" ) )
            .def( py::init( []( std::pair< rofi::messages::Packet, int64_t > arg ) {
                return PacketFilter::DelayedPacket{ .packet = std::move( arg.first ),
                                                    .delay = std::chrono::milliseconds(
                                                            arg.second ) };
            } ) );
}

PyFilter::PyFilter( const std::string & filterCode )
        : _pyGuard( GlobalPyInterpreterGuard::instance() )
        , _scope( py::module_::import( "rofipacketf" ).attr( "__dict__" ) )
{
    py::exec( filterCode, py::globals(), _scope );

    if ( !_scope.contains( "filter" ) ) {
        throw std::logic_error( "Python packet filter file has to contain `filter` definition" );
    }
}

auto PyFilter::filter( PacketFilter::SendPacketData packetData ) -> PacketFilter::DelayedPacket
{
    using namespace py::literals;

    assert( _scope.contains( "filter" ) );
    auto result = _scope[ "filter" ]( "packet"_a = packetData.packet,
                                      "sender"_a = packetData.sender,
                                      "receiver"_a = packetData.receiver );
    // auto result = py::eval( "DelayedPacket(filter(None, None, None))", py::globals(), _scope );

    if ( result.is_none() ) {
        // Packet lost
        return PacketFilter::DelayedPacket{ .packet = std::move( packetData.packet ),
                                            .delay = std::chrono::milliseconds( -1 ) };
    }

    if ( result.is( py::type::of< PacketFilter::DelayedPacket >() ) ) {
        return result.cast< PacketFilter::DelayedPacket >();
    }

    // return py::class_< PacketFilter::DelayedPacket >( std::move( result ) )
    //         .cast< PacketFilter::DelayedPacket >();

    assert( _scope.contains( "DelayedPacket" ) );
    return _scope[ "DelayedPacket" ]( std::move( result ) ).cast< PacketFilter::DelayedPacket >();
}

} // namespace rofi::simplesim::packetf
