add_rofi_firmware(wifi_example.elf main.cpp)
target_link_libraries(wifi_example.elf PUBLIC rofi::hal libfi)
set_partition_table(wifi_example.elf partitions.csv)

# Load secrets from env
if ("$ENV{ROFI_SSID}" STREQUAL "")
    message(FATAL_ERROR "Missing WiFi configuration in the environment"
        "Please set: ROFI_SSID, ROFI_WIFI_PASS, ROFI_WLAN_USER, ROFI_WLAN_PASS")
endif()

target_compile_definitions(wifi_example.elf PRIVATE
    "SSID=\"$ENV{ROFI_SSID}\""
    "PASSWORD=\"$ENV{ROFI_WIFI_PASS}\""
    "WLAN_USER=\"$ENV{ROFI_WLAN_USER}\""
    "WLAN_PASS=\"$ENV{RoFI_WLAN_PASS}\"")