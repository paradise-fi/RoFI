#pragma once

#include <vector>
#include <z3++.h>

namespace rofi::smtr {

template < typename Self, template< typename > class crtpType >
struct Crtp
{
    Self& underlying() { return static_cast< Self& >( *this ); }
    Self const& underlying() const { return static_cast< Self const& >( *this ); }
private:
    friend crtpType< Self >;
};

} // namespace rofi::smtr
