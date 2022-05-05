#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <rofi_hal.hpp>

namespace py = pybind11;

using namespace rofi::hal;

PYBIND11_MODULE( pyRofiHal, m ) {
    m.doc() = "RoFI simulator HAL";

    py::class_< Joint > joint( m, "Joint" );
    joint
        .def_property_readonly( "maxPosition", &Joint::maxPosition )
        .def_property_readonly( "minPosition", &Joint::minPosition )
        .def_property_readonly( "maxSpeed", &Joint::maxSpeed )
        .def_property_readonly( "minSpeed", &Joint::minSpeed )
        .def_property_readonly( "maxTorque", &Joint::maxTorque )
        .def_property_readonly( "velocity", &Joint::getVelocity )
        .def_property_readonly( "torque", &Joint::getTorque )
        .def_property_readonly( "position", &Joint::getPosition )
        .def( "setVelocity", &Joint::setVelocity )
        .def( "setTorque", &Joint::setTorque )
        .def( "setPosition", &Joint::setPosition )
        .def( "onError", &Joint::onError );

    py::enum_< Joint::Error >( joint, "Error" )
        .value( "Communication", Joint::Error::Communication )
        .value( "Hardware", Joint::Error::Hardware )
        .export_values();

    py::enum_< ConnectorPosition >( m, "ConnectorPosition" )
        .value( "Retracted", ConnectorPosition::Retracted )
        .value( "Extended", ConnectorPosition::Extended )
        .export_values();

    py::enum_< ConnectorOrientation >( m, "ConnectorOrientation" )
        .value( "North", ConnectorOrientation::North )
        .value( "East", ConnectorOrientation::East )
        .value( "South", ConnectorOrientation::South )
        .value( "West", ConnectorOrientation::West )
        .export_values();

    py::enum_< ConnectorLine >( m, "ConnectorLine" )
        .value( "Internal", ConnectorLine::Internal )
        .value( "External", ConnectorLine::External )
        .export_values();

    py::enum_< ConnectorEvent >( m, "ConnectorEvent" )
        .value( "Connected", ConnectorEvent::Connected )
        .value( "Disconnected", ConnectorEvent::Disconnected )
        .value( "PowerChanged", ConnectorEvent::PowerChanged )
        .export_values();

    py::class_< ConnectorState >( m, "ConnectorState" )
        .def_readwrite( "position", &ConnectorState::position )
        .def_readwrite( "internal", &ConnectorState::internal )
        .def_readwrite( "external", &ConnectorState::external )
        .def_readwrite( "connected", &ConnectorState::connected )
        .def_readwrite( "orientation", &ConnectorState::orientation );

    py::class_< Connector >( m, "Connector" )
        .def_property_readonly( "state", &Connector::getState )
        .def( "connect", &Connector::connect )
        .def( "disconnect", &Connector::disconnect )
        .def( "onConnectorEvent", &Connector::onConnectorEvent )
        .def( "onPacket", &Connector::onPacket )
        .def( "send", &Connector::send )
        .def( "connectPower", &Connector::connectPower )
        .def( "disconnectPower", &Connector::disconnectPower );

    py::class_< RoFI > rofiC( m, "RoFI" );
    rofiC
        .def_property_readonly( "id", &RoFI::getId )
        .def( "getJoint", &RoFI::getJoint )
        .def( "getConnector", &RoFI::getConnector )
        .def_property_readonly( "descriptor", &RoFI::getDescriptor )
        .def( "wait", &RoFI::wait )
        .def_static( "getLocal", &RoFI::getLocalRoFI )
        .def_static( "getRemote", &RoFI::getRemoteRoFI );

    py::class_< RoFI::Descriptor >( rofiC, "Descriptor" )
        .def_readwrite( "jointCount", &RoFI::Descriptor::jointCount )
        .def_readwrite( "connectorCount", &RoFI::Descriptor::connectorCount );
}
