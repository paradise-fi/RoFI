add_rofi_firmware(umGridui.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(umGridui.elf
    idf::freertos idf::spi_flash idf::console rofi::hal::esp32_jig rofi::hal::js jac-device idf::vfs idf::fatfs idf::pthread rofi::hal::rbprotocol rofi::hal::gridui)
set_partition_table(umGridui.elf partitions.csv)

# Load secrets from env
if ("$ENV{ROFI_SSID}" STREQUAL "")
    message(FATAL_ERROR "Missing WiFi configuration in the environment"
        "Please set: ROFI_SSID, ROFI_WIFI_PASS, ROFI_WLAN_USER, ROFI_WLAN_PASS")
endif()

target_compile_definitions(umGridui.elf PRIVATE
    "SSID=\"$ENV{ROFI_SSID}\""
    "PASSWORD=\"$ENV{ROFI_WIFI_PASS}\""
    "WLAN_USER=\"$ENV{ROFI_WLAN_USER}\""
    "WLAN_PASS=\"$ENV{RoFI_WLAN_PASS}\"")

# build image, but not accessible from ESP device - like not flashed or generated successfully
spiffs_create_spiflash_image(umGridui.elf "spiffs" "spiffsData" 184320)