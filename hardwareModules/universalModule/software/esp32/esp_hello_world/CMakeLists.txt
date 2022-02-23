add_rofi_firmware(esp_hello_world.elf
    FLASH_SIZE 4MB
    SOURCES main.c)
target_link_libraries(esp_hello_world.elf idf::esp32 idf::freertos idf::spi_flash)
set_partition_table(esp_hello_world.elf partitions.csv)
