#pragma once
#include "lwip++.hpp"
#include "messagingResult.hpp"

enum CallbackType
{
    TaskRequestCb,
    TaskFailureCb,
    CustomMessageCb,
    CustomMessageBlockingCb,
    CompleteBlockingMessageCb,
};

class UserCallbackInvoker
{
public:
    virtual MessagingResult invokeUserCallback( CallbackType cbType, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) = 0;
};