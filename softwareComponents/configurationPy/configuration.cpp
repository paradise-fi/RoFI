#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <configuration/rofibot.hpp>
#include <carma>
#include <atoms/containers.hpp>
#include <atoms/units.hpp>
#include <atoms/util.hpp>
#include <armadillo>

namespace py = pybind11;
using namespace rofi::configuration;
using namespace rofi::configuration::matrices;

PYBIND11_MODULE(pyConfiguration, m) {
	m.doc() = "This is the module docs.";

	py::enum_<ComponentType>(m, "ComponentType")
        .value("UmShoe", ComponentType::UmShoe)
        .value("UmBody", ComponentType::UmBody)
        .value("Roficom", ComponentType::Roficom)
        .export_values();

    py::enum_<ModuleType>(m, "ModuleType")
        .value("Unknown", ModuleType::Unknown)
        .value("Universal", ModuleType::Universal)
        .value("Pad", ModuleType::Pad)
        .export_values();

    py::enum_<roficom::Orientation>(m, "Orientation")
        .value("North", roficom::Orientation::North)
        .value("East", roficom::Orientation::East)
        .value("South", roficom::Orientation::South)
        .value("West", roficom::Orientation::West)
        .export_values();

    m.def(
        "orientationToAngle",
        &roficom::orientationToAngle,
        "Convert orientation to radians.",
        py::arg("o") = roficom::Orientation::North
    );

    m.def(
        "orientationToTransform",
        &roficom::orientationToTransform,
        "Convert orientation to transform"
    );

    m.def(
        "orientationToString",
        &roficom::orientationToString,
        "Convert orientation to string."
    );

    m.def(
        "stringToOrientation",
        &roficom::stringToOrientation,
        "Convert string to orientation enum."
    );

    /*py::class_<ComponentJoint>(m, "ComponentJoint")
        //.def(py::init<atoms::ValuePtr<Joint>&, int&, int&>())
        .def_readwrite( "joint", &ComponentJoint::joint )
        .def_readwrite( "sourceComponent", &ComponentJoint::sourceComponent )
        .def_readwrite( "destinationComponent", &ComponentJoint::sourceComponent );*/


    py::class_<RoficomJoint>(m, "RoficomJoint");
    py::class_<SpaceJoint>(m, "SpaceJoint");
    py::class_<Rofibot>(m, "Rofibot");
    py::class_<Module>(m, "Module");
    //py::class_<arma::Mat<double>::fixed<4ull, 4ull>>(m, "Matrix");
    /*py::class_<Component>(m, "Component")
        .def(py::init<ComponentType&, std::vector<int>&, std::vector<int>&, *(Module&)>())
        .def_readwrite( "type", &Component::type )
        .def_readwrite( "inJoints", &Component::inJoints )
        .def_readwrite( "outJoints", &Component::outJoints );*/
}