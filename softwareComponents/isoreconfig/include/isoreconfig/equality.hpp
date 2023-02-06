#include <configuration/rofiworld.hpp>

namespace rofi::isoreconfig {

bool equalConfiguration( const rofi::configuration::RofiWorld& rw1, const rofi::configuration::RofiWorld& rw2 );

struct StrictRofiWorldEquality 
{
    bool operator()( const rofi::configuration::RofiWorld& rw1, const rofi::configuration::RofiWorld& rw2 ) const
    {
        return equalConfiguration( rw1, rw2 );
    }
};

} // namespace rofi::isoreconfig
