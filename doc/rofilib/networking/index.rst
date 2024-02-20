Netwokring
==========

Networking is available via `lwip <https://www.nongnu.org/lwip/2_1_x/index.html>` library onto which
platform's networking is built. By default, IPv6 protocol is used.

The networking is represented by the class :cpp:class:`NetworkManager <rofi::net::NetworkManager>`
which enables user not only an easier control over network interfaces, their configuration, etc.,
but also provides additional functionality such as routing support. There should be always one
instance of the class running within a single module as the `NetworkManager` takes care of all
its connectors -- within the networking, they are represented with :cpp:class:`Interface <rofi::net::Interface>`
-- and there is an additional virtual interface (`"rl0"`) independent of physical connectors used as the main
interface of the module. This virtual interface is the interface you want to configure, if you don't
know which one.

The usage is simple, the class itself initializes everything using :cpp:class:`RoFI <rofi::hal::RoFI>`.
For example, let us initialize networking of a module and assign an IPv6 address to it.

.. code-block:: cpp

    #include <networking/networkManager.hpp>

    // ...
    // if lwip's tcpip stack is not initialized, call
    // tcpip_init( nullptr, nullptr );

    hal::RoFI localRofi = RoFI::getLocalRoFI();

    NetworkManager net( localRofi );

    // adds address fc07:0:0:1::1 with mask /80 to interface "rl0", which is the virtual one
    net.addAddress( "fc07:0:0:1::1"_ip, 80, net.interface( "rl0" ) );
    // set all interfaces up -- to process incomming/outcomming traffic
    net.setUp();


Now, we have a module with an IP address configured. Suppose, that we have a network of interconnected
modules where each has its own address. To be able to send a message between two modules, the routing
table of a receiver (and all modules in-between) has to contain a path to receiver. To populate the routing
table, the routing protocol must be configured.

The RoFI platform offers multiple routing protocols by default; these are
:cpp:class:`SimplePeriodic <rofi::net::SimplePeriodic>`, :cpp:class:`SimpleReactive <rofi::net::Reactive>`,
and :cpp:class:`RRP <rofi::net::RRP>`. The first two protocols are very simple and they always share their whole
routing table. The main difference is the way messages are sent; the first one sends updates periodically, the second
one sends messages on network changes only. The third protocol tries to be smarter about sharing routing information;
more details can be found in the Protocols section.

Using `SimpleReactive` protocol, we enable routing with

.. code-block:: cpp

    #include <networking/protocols/simpleReactive.hpp>

    // the code from above

    // Add protocol into the NetworkManager so that it can be used
    auto* simpleReactive = net.addProtocol( SimpleReactive() );

    // now we enable the protocol on all interfaces
    net.setProtocol( simpleReactive );
    // you can also enable the protocol only on certain interfaces
    // e.g., to enable the protocol on interface "rd4", you can call
    //     net.setProtocol( simpleReactive, net.getInterface( "rd4" ) );
    //
    // you could also accomplish the same as above with a for loop
    //    for ( const auto& i : net.interfaces() ) {
    //        net.setProtocol( simpleReactive, net.interface( i.name() ) );
    //    }
    //

The routing process is now running, so the interconnected modules will share necessary information and populate the routing
table. Then you can use regular sockets from `lwip` library and send messages accross the network.

Network Manager
---------------

.. doxygenclass:: rofi::net::NetworkManager
    :project: networking

Protocols
---------

The interface is desribed by the class :cpp:class`Protocol <rofi::net::Protocol>`

.. doxygenclass:: rofi::net::Protocol
    :project: networking

And there are some default implementations. For routing we have

.. doxygenclass:: rofi::net::SimplePeriodic
    :project: networking

.. doxygenclass:: rofi::net::SimpleReactive
    :project: networking

.. doxygenclass:: rofi::net::RRP
    :project: networking

and there is also a simple leader elect protocol available

.. doxygenclass:: rofi::net::LeaderElect
    :project: networking

Command Line Interface (CLI)
----------------------------

Simple CLI interface for the network management is availabe.

.. doxygenclass:: rofi::net::NetworkManagerCli
    :project: networking

Components
----------

The rest of components are listed here. The user will encouter them, but changes in their configuration
should be done indirectly via `NetworkManager`.

.. toctree::
    interface
    routingTable

