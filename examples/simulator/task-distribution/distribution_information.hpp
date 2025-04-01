#include <lwip++.hpp>
#include <lwip/udp.h>
#include <networking/networkManager.hpp>
#include "task.hpp"

using namespace rofi::net;

class DistributionInformation
{
public:
    DistributionInformation( Ip6Addr sendTo, TaskBase* task )
    : _sendTo( sendTo ), _task( task ) {}

    const Ip6Addr target() const { return _sendTo; }
    TaskBase* task() { return _task.get(); }

private:
    Ip6Addr _sendTo;
    std::unique_ptr< TaskBase > _task;
};