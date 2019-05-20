#pragma once

#include <stm32g0xx_hal_crc.h>
#include <cassert>

class Crc {
public:
    static uint32_t compute( uint8_t *begin, int length ) {
        return HAL_CRC_Calculate( &instance()._periph,
            reinterpret_cast< uint32_t *>( begin ), length );
    }
private:
    Crc() {
        _periph.Instance = CRC;
        _periph.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
        _periph.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
        _periph.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
        _periph.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
        _periph.Init.CRCLength = CRC_POLYLENGTH_32B;
        _periph.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
        if ( HAL_CRC_Init( &_periph ) != HAL_OK ) {
            assert( false && "CRC setup failed" );
        }
    }

    static Crc& instance() {
        static Crc crc;
        return crc;
    }

    CRC_HandleTypeDef _periph;
};