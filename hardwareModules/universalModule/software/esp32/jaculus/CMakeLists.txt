add_rofi_firmware(umJaculus.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(umJaculus.elf
    idf::freertos idf::spi_flash idf::console rofi::hal rofi::hal::js jac-device idf::vfs idf::fatfs idf::pthread)
set_partition_table(umJaculus.elf partitions-4m.csv)

target_compile_definitions(umJaculus.elf PRIVATE "JAC_ESP32_VERSION=\"0.0.5\"")

# build image, but not accessible from ESP device - like not flashed or generated successfully
# returns error: Failed to create FATFS image for partition 'umJaculus.elf'.  Check project configuration if using the correct partition table file.

message(STATUS "Creating spiflash image for umJaculus.elf")
# fatfs_create_spiflash_image_dir(umJaculus.elf "storage" "strData" 0x93000) # last argument is size of partition in bytes - from partition table
