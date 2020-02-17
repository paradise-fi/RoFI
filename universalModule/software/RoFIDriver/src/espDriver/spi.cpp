#include "spi.hpp"

std::ostream& operator<<( std::ostream& o, spi_bus_config_t s ) {
    o << "mosi_io_num: " << s.mosi_io_num << "\n";
    o << "miso_io_num: " << s.miso_io_num << "\n";
    o << "sclk_io_num: " << s.sclk_io_num << "\n";
    o << "quadwp_io_num: " << s.quadwp_io_num << "\n";
    o << "quadhd_io_num: " << s.quadhd_io_num << "\n";
    o << "max_transfer_sz: " << s.max_transfer_sz << "\n";
    return o;
}

std::ostream& operator<<( std::ostream& o, spi_device_interface_config_t s ) {
    o << "command_bits: " << static_cast< int >( s.command_bits ) << "\n";
    o << "address_bits: " << static_cast< int >( s.address_bits ) << "\n";
    o << "dummy_bits: " << static_cast< int >( s.dummy_bits ) << "\n";
    o << "mode: " << static_cast< int >( s.mode ) << "\n";
    o << "duty_cycle_pos: " << static_cast< int >( s.duty_cycle_pos ) << "\n";
    o << "cs_ena_pretrans: " << static_cast< int >( s.cs_ena_pretrans ) << "\n";
    o << "cs_ena_posttrans: " << static_cast< int >( s.cs_ena_posttrans ) << "\n";
    o << "clock_speed_hz: " << s.clock_speed_hz << "\n";
    o << "spics_io_num: " << s.spics_io_num << "\n";
    o << "flags: " << s.flags << "\n";
    o << "queue_size: " << s.queue_size << "\n";
    o << "pre_cb: " << s.pre_cb << "\n";
    o << "post_cb: " << s.post_cb << "\n";
    return o;
}
