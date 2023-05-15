
/* 
* This file is part of VL53L1 Platform 
* 
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved 
* 
* License terms: BSD 3-clause "New" or "Revised" License. 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this 
* list of conditions and the following disclaimer. 
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution. 
* 
* 3. Neither the name of the copyright holder nor the names of its contributors 
* may be used to endorse or promote products derived from this software 
* without specific prior written permission. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
* 
*/

#include "vl53l1_platform.h"
#include <string.h>
#include <time.h>
#include <math.h>

#include <array>
#include <functional>
#include <span>

#include <stm32g0xx_ll_i2c.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_exti.h>
#include <stm32g0xx_ll_utils.h> // LL_mDelay

#include <drivers/gpio.hpp>
#include <drivers/i2c.hpp>
#include <drivers/timer.hpp>

#include "vl53l1_platform.h"

#include <string.h>
#include <time.h>
#include <math.h>

#include <lidar.hpp>

#include <cassert>

// Anonymous namespace is to hide symbols only into this compilation unit.
namespace {
I2C* pI2c = nullptr;
}

void lidar_inner::initialize_platform( I2C* i2cPeriph ) {
	pI2c = i2cPeriph;
}

template< std::size_t N >
inline static int8_t _WriteMulti( uint16_t slaveAddress, uint16_t registerAddress, uint8_t *transmitBuffer ) {
	constexpr auto addressSize = 2;

	// NOTE: register address must be sent with data in one transaction 
 	std::array< uint8_t, addressSize + N > buffer = {
		static_cast<uint8_t>( registerAddress >> 8 ),
		static_cast<uint8_t>( registerAddress & 0xFF ) 
	};

	if constexpr ( N > 0 )
		std::copy( transmitBuffer, transmitBuffer + N, std::next( buffer.begin(), addressSize ) );

	assert( pI2c );

	return pI2c->write( slaveAddress, buffer );
}

inline static int8_t _WriteRegisterAddress( uint16_t slaveAddress, uint16_t registerAddress ) {
	return _WriteMulti< 0 >( slaveAddress, registerAddress, nullptr );
}

template< std::size_t N >
inline static int8_t _ReadMulti( uint16_t slaveAddress, uint16_t registerAddress, uint8_t *transmitBuffer ) {
	VL53L1X_ERROR status = _WriteRegisterAddress( slaveAddress, registerAddress );
	if ( status != 0 ) {
		return status;
	}

	auto span = std::span< uint8_t, N >( transmitBuffer, N );
	
	assert( pI2c );
	
	return pI2c->read( slaveAddress, span );
}

// TODO: this is not used by the current ULD but declared in the interface
int8_t VL53L1_WriteMulti( uint16_t slaveAddress, uint16_t registerAddress, uint8_t *transmitBuffer, uint32_t bufferSize) {
	/**
	 * This is a workaround for sending the address bytes as first 2 (without compile time size knowledge)
	 * with using already specified functionality of i2c.
	 * 
	 * NOTE: this function is effective when the `bufferSize >= 8` 
	 */
	struct bufferWithAddress {
		uint8_t *transmitBuffer;
		uint32_t bufferSize;
		std::array< uint8_t, 2 > address;

		uint8_t operator[]( size_t index ) const {
			return index < address.size() ? address[ index ] : transmitBuffer[ index - address.size() ];
		}

		size_t size() const {
			return address.size() + bufferSize;
		}
	};

	const bufferWithAddress buffer = {
		transmitBuffer,
		bufferSize,
		{
			static_cast<uint8_t>( registerAddress >> 8 ),
			static_cast<uint8_t>( registerAddress & 0xFF ) 
		},
	};

	assert( pI2c );

	return pI2c->unsafe_write( slaveAddress, buffer, buffer.size() );
}

int8_t VL53L1_ReadMulti(uint16_t slaveAddress, uint16_t registerAddress, uint8_t *transmitBuffer, uint32_t bufferSize) {
	VL53L1X_ERROR status = _WriteRegisterAddress( slaveAddress, registerAddress );
	if ( status != 0 ) {
		return status;
	}

	assert( pI2c);

	return pI2c->unsafe_read( slaveAddress, transmitBuffer, bufferSize );
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t registerAddress, uint8_t data) {
	return _WriteMulti< 1 >( dev, registerAddress, &data );
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t registerAddress, uint16_t data) {
	uint8_t transmitBuffer[2] = {
		static_cast<uint8_t>( data >> 8 ),
		static_cast<uint8_t>( data & 0xFF )
	};
	
	return _WriteMulti< sizeof(transmitBuffer) >( dev, registerAddress, transmitBuffer );
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t registerAddress, uint32_t data) {
	uint8_t transmitBuffer[4] = {
		static_cast<uint8_t>( data >> 24 ),
		static_cast<uint8_t>( data >> 16 ),
		static_cast<uint8_t>( data >> 8 ),
		static_cast<uint8_t>( data & 0xFF )
	};
	
	return _WriteMulti< sizeof(transmitBuffer) >( dev, registerAddress, transmitBuffer );
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t registerAddress, uint8_t *data) {
	return _ReadMulti< 1 >( dev, registerAddress, data );
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t registerAddress, uint16_t *data) {
	uint8_t transmitBuffer[2] = { 0, 0 };
	int8_t status = _ReadMulti< sizeof( transmitBuffer ) >( dev, registerAddress, transmitBuffer );

	*data = transmitBuffer[0] << 8;
	*data += transmitBuffer[1];
	
	return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t registerAddress, uint32_t *data) {
	uint8_t transmitBuffer[4] = { 0, 0, 0, 0 };
	int8_t status = _ReadMulti< sizeof( transmitBuffer ) >( dev, registerAddress, transmitBuffer );

	*data = transmitBuffer[0] << 24;
	*data += transmitBuffer[1] << 16;
	*data += transmitBuffer[2] << 8;
	*data += transmitBuffer[3];
	
	return status;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms) {
	LL_mDelay(wait_ms);

	return 0;
}
