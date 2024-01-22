#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_console.h>

#include <string_view>
#include <iostream>
#include <sstream>
#include <optional>

#include <atoms/util.hpp>
#include <atoms/units.hpp>

#include <rofi_hal.hpp>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/eventLoopFeature.h>
#include <jac/features/timersFeature.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>
#include <jac/features/basicStreamFeature.h>
#include <jac/features/stdioFeature.h>

#include <jac/device/device.h>
#include <jac/device/logger.h>
#include <jac/features/util/linkIo.h>

#include <jac/link/mux.h>
#include <jac/link/encoders/cobs.h>


#include "espFeatures/rofiFeature.h"
#include "espFeatures/freeRTOSEventQueue.h"

#include "util/uartStream.h"

#include <string>
#include <filesystem>
#include <sstream>

#include "esp_vfs_fat.h"
#include "freertos/task.h"

#include "esp_pthread.h"

#include "platform/esp32.h"

using namespace std::literals;

// // This is a simple utility, so let's make things simple and use global objects
// // so we can reference it from plain C functions required by CLI interface
// std::optional< rofi::hal::RoFI > localRoFI;
// float speedCoef = 1;

wl_handle_t storage_wl_handle = WL_INVALID_HANDLE;

using Machine = jac::ComposeMachine<
    jac::MachineBase,
    FreeRTOSEventQueueFeature,
    jac::BasicStreamFeature,
    jac::StdioFeature,
    PlatformInfoFeature,
    jac::FilesystemFeature,
    jac::ModuleLoaderFeature,
    jac::EventLoopFeature,
    RofiFeature,
    jac::TimersFeature,
    jac::EventLoopTerminal
>;

jac::Device<Machine> device(
    "/data",
    []() { // get memory stats
        std::stringstream oss;
        oss << esp_get_free_heap_size() << "/" << esp_get_minimum_free_heap_size();
        return oss.str();
    },
    []() { // get storage stats
        // std::stringstream oss;
        // auto stats = std::filesystem::space("/data");
        // oss << "Storage usage: \n  " << stats.available << "/" << stats.capacity << "\n";
        // return oss.str();
        return "not implemented";
    },
    {{"esp32", JAC_ESP32_VERSION}}, // version info
    [](std::filesystem::path path) { // format storage
        jac::Logger::debug("Formatting storage");

        esp_vfs_fat_spiflash_unmount_rw_wl("/data", storage_wl_handle);

        auto* partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
        if (partition == nullptr) {
            return;
        }
        esp_partition_erase_range(partition, 0, partition->size);
    }
);

using Mux_t = jac::Mux<jac::CobsEncoder>;
std::unique_ptr<Mux_t> muxUart;


void reportMuxError(jac::MuxError error, std::any ctx) {
    std::string message = "Mux error: ";
    switch (error) {
        case jac::MuxError::INVALID_RECEIVE:
            {
                auto& ref = std::any_cast<std::tuple<int, uint8_t>&>(ctx);
                message += "INVALID_RECEIVE " + std::to_string(std::get<0>(ref));
                message += " [" + std::to_string(std::get<1>(ref)) + "]";
                jac::Logger::debug(message);
            }
            break;
        case jac::MuxError::PACKETIZER:
            {
                auto& ref = std::any_cast<int&>(ctx);
                message += "PACKETIZER_ERROR " + std::to_string(ref);
                jac::Logger::debug(message);
            }
            break;
        case jac::MuxError::PROCESSING:
            {
                auto& ref = std::any_cast<std::string&>(ctx);
                message += "PROCESSING_ERROR '" + ref + "'";
                jac::Logger::error(message);
            }
            break;
    }
}

int main() {
    // Initialize vfs
    esp_vfs_fat_mount_config_t conf = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .disk_status_check_enable = false
    };

    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl("/data", "storage", &conf, &storage_wl_handle));

    // initialize uart connection
    auto uartStream = std::make_unique<UartStream>(UART_NUM_0, 921600, 4096, 0);
    uartStream->start();

    muxUart = std::make_unique<Mux_t>(std::move(uartStream));
    muxUart->setErrorHandler(reportMuxError);
    auto handleUart = device.router().subscribeTx(1, *muxUart);
    muxUart->bindRx(std::make_unique<decltype(handleUart)>(std::move(handleUart)));


    device.onConfigureMachine([&](Machine &machine) {
        device.machineIO().in->clear();

        machine.stdio.out = std::make_unique<jac::LinkWritable>(device.machineIO().out.get());
        machine.stdio.err = std::make_unique<jac::LinkWritable>(device.machineIO().err.get());
        machine.stdio.in = std::make_unique<jac::LinkReadable<Machine>>(&machine, device.machineIO().in.get());

        esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
        cfg.stack_size = 8 * 1024;
        cfg.inherit_cfg = true;
        esp_pthread_set_cfg(&cfg);
    });

    esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
    cfg.stack_size = 20 * 1024;
    cfg.inherit_cfg = true;
    esp_pthread_set_cfg(&cfg);

    device.start();

    device.startMachine("index.js");
}


extern "C" void app_main() {
    main();

    vTaskDelete(NULL);
}