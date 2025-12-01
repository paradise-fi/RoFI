#pragma once
#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <iostream>
#include <string>

using namespace rofi::hal;
using namespace rofi::net;

Ip6Addr createAddress( int id ) {
    std::stringstream ss;
    ss << "fc07:0:0:";
    ss << id;
    ss << "::1";

    return Ip6Addr( ss.str() );
}