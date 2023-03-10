add_rofi_firmware(firmware_updater.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(firmware_updater.elf PRIVATE rofi::hal rofi::firmwareUpdater)
set_partition_table(firmware_updater.elf partitions.csv)
