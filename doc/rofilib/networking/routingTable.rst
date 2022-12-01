Routing table
-------------

The routing table contains all routes known to the networking within the `NetworkManager` of given
module. The table itself manages underlying lwip's forwarding table (an addition of the RoFI
platform) which should be left intact by the user.

.. doxygenclass:: rofi::net::RoutingTable
    :project: networking
