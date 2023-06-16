add_rofi_firmware(umJaculus.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(umJaculus.elf
    idf::freertos idf::spi_flash idf::console rofi::hal rofi::hal::js jac-device idf::vfs idf::fatfs idf::pthread)
set_partition_table(umJaculus.elf partitions-4m.csv)

# have to be implemented using cmake
# fatfs_create_spiflash_image("storage" "./data")