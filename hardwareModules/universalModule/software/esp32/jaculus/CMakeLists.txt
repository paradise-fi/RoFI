add_rofi_firmware(umJaculus.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(umJaculus.elf
    idf::freertos idf::spi_flash idf::console rofi::hal rofi::hal::js)
set_partition_table(umJaculus.elf partitions-8m.csv)
