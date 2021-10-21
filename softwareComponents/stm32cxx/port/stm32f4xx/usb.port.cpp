#include <drivers/usb.hpp>

extern "C" void OTG_FS_IRQHandler() {
    UsbDevice::instance()._onInterrupt();
}
