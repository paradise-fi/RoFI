#pragma once

#include <stdint.h>

template <class T>
constexpr T Bitmask(uint8_t arg) {
	return T(1) << arg;
}

template <class T, class...Args>
constexpr T Bitmask(uint8_t arg, Args...args) {
	return (T(1) << arg) | Bitmask<T>(args...);
}

template <class...Args>
constexpr uint8_t Bitmask8(Args...args) {
	return Bitmask<uint8_t>(args...);
}

template <class...Args>
constexpr uint16_t Bitmask16(Args...args) {
	return Bitmask<uint16_t>(args...);
}

template <class...Args>
constexpr uint32_t Bitmask32(Args...args) {
	return Bitmask<uint32_t>(args...);
}
