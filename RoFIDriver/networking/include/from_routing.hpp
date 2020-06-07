#pragma once

#include <lwip/ip.h>
#include <sstream>
#include <iostream>

std::string chunk_to_rest(uint8_t chunk) {
	switch (chunk) {
		case 3:
			return "e::";
		case 2:
			return "c::";
		case 1:
			return "8::";
		case 0:
			return ":";
		default:
			std::cerr << "chunk_to_rest got something wrong\n";
			return "";
	}
}

void mask_to_address(uint8_t mask, ip6_addr_t* m) {
	if (mask == 128) {
		ip6addr_aton("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", m);
		return;
	} else if (mask == 0) {
		ip6_addr_set_zero(m);
		return;
	}

	std::ostringstream s;

	while (mask >= 16) {
		s << "ffff:";
		mask = mask - 16;
	}

	while (mask > 4) {
		s << "f";
		mask = mask - 4;
		if (mask == 0) {
			s << ":";
		}
	}

	s << chunk_to_rest(mask);
	ip6addr_aton(s.str().c_str(), m);
}