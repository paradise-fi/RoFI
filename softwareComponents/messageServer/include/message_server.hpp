#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <rofi/gz_client.hpp>


namespace rofi::msgs
{
using rofi::gz::getCStyleArgs;

using rofi::gz::Client;
using rofi::gz::Server;

} // namespace rofi::msgs
