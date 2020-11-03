#pragma once


namespace rofi::networking
{
class InternalState
{
public:
    using RofiId = int;

    static std::string getTopic( RofiId rofiId )
    {
        using std::to_string;
        return "rofi_" + to_string( rofiId );
    }
};

} // namespace rofi::networking
