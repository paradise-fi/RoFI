#include <drivers/usb.hpp>

extern "C" void USB_LP_IRQHandler() {
    UsbDevice::instance()._onInterrupt();
}
