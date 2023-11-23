#include "session_id.hpp"

#include <boost/uuid/random_generator.hpp>


#ifndef SESSION_ID
    #define SESSION_ID boost::uuids::random_generator()()
#endif

using rofi::hal::SessionId;

const SessionId & SessionId::get()
{
    static SessionId instance = SessionId( SESSION_ID );
    return instance;
}
