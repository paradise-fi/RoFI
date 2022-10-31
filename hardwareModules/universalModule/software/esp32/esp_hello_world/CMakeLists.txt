add_rofi_firmware(esp_hello_world.elf
    FLASH_SIZE 4MB
    SOURCES main.c)
target_link_libraries(esp_hello_world.elf idf::freertos idf::spi_flash
                      idf::esp_hw_support idf::esp_rom)
set_partition_table(esp_hello_world.elf partitions.csv)
