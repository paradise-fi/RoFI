#include <configuration/rofiworld.hpp>

namespace rofi::isoreconfig {

using namespace rofi::configuration;

bool equalConfiguration( const RofiWorld& rw1, const RofiWorld& rw2 );

struct StrictRofiWorldEquality 
{
    bool operator()( const RofiWorld& rw1, const RofiWorld& rw2 ) const
    {
        return equalConfiguration( rw1, rw2 );
    }
};

} // namespace rofi::isoreconfig