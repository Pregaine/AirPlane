

/*
* Copyright (C) 2011-2012 Bitcrazy AB
* Adapted to Cortex-M4 Fly Controller by Nuvoton
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, in version 3.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/
/*============================================================================*
* O 	O		   __					______	__							 *
*  \   /	  /\  / /_		_	 __    / /___/ / /_ 	_					 *
*	[+] 	 /	\/ / \\    //__ / /__ / /____ / / \\   //					 *
*  /   \	/ /\  /   \\__// --/ /---/ /----// /   \\_//					 *
* O 	O  /_/	\/	   \__/    \_\/ /_/ 	/_/ ____/_/ 					 *
*																			 *
*																			 *
* Multi-Rotor controller firmware for Nuvoton Cortex M4 series				 *
*																			 *
* Written by by T.L. Shen for Nuvoton Technology.							 *
* tlshen@nuvoton.com/tzulan611126@gmail.com 								 *
*																			 *
*============================================================================*
*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nvt_i2c.h"

void I2C_Init()
{

    NVT_I2C_Init();
}


int8_t I2C_readBytes( uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t * data, uint16_t timeout )
{
    uint8_t ErrorFlag;

    NVT_SetDeviceAddress( devAddr );
    ErrorFlag = NVT_ReadByteContinue_addr8( regAddr, data, length, timeout );

#ifdef I2CDEV_SERIAL_DEBUG
    Serial.print( ". Done (" );
    Serial.print( count, DEC );
    Serial.println( " read)." );
#endif

    if( ErrorFlag )
    {
        return 0;

    }


    else
    {
        return 1;
    }


}


int8_t I2C_readByte( uint8_t devAddr, uint8_t regAddr, uint8_t * data, uint16_t timeout )
{
    return I2C_readBytes( devAddr, regAddr, 1, data, timeout );
}


int8_t I2C_readBits( uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t * data, 
    uint16_t timeout )
{
    uint8_t count, b;

    if( ( count = I2C_readByte( devAddr, regAddr, &b, timeout ) ) != 0 )
    {
        uint8_t mask = ( ( 1 << length ) - 1 ) << ( bitStart - length + 1 );

        b &= mask;
        b >>= ( bitStart - length + 1 );
        *data = b;
    }


    return count;
}


int8_t I2C_readBit( uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t * data, uint16_t timeout )
{
    uint8_t b;
    uint8_t count = I2C_readByte( devAddr, regAddr, &b, timeout );

    *data = b & ( 1 << bitNum );
    return count;
}


int8_t I2C_readWords( uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t * data, uint16_t timeout )
{
    return 0;
}


int8_t I2C_readWord( uint8_t devAddr, uint8_t regAddr, uint16_t * data, uint16_t timeout )
{
    return I2C_readWords( devAddr, regAddr, 1, data, timeout );
}


int8_t I2C_readBitW( uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t * data, uint16_t timeout )
{
    uint16_t b = 0;
    uint8_t count = I2C_readWord( devAddr, regAddr, &b, timeout );

    *data = b & ( 1 << bitNum );

    return count;
}


int8_t I2C_readBitsW( uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t * data, 
    uint16_t timeout )
{
    uint8_t count;
    uint16_t w;

    if( ( count = I2C_readWord( devAddr, regAddr, &w, timeout ) ) != 0 )
    {
        uint16_t mask = ( ( 1 << length ) - 1 ) << ( bitStart - length + 1 );

        w &= mask;
        w >>= ( bitStart - length + 1 );
        *data = w;
    }


    return count;
}


bool I2C_writeBytes( uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t * data )
{
    NVT_SetDeviceAddress( devAddr );
    NVT_WriteByteContinue_addr8( regAddr, data, length );
    return 1;
}


bool I2C_writeByte( uint8_t devAddr, uint8_t regAddr, uint8_t data )
{
    return I2C_writeBytes( devAddr, regAddr, 1, &data );
}


bool I2C_writeBit( uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data )
{
    uint8_t b;

    I2C_readByte( devAddr, regAddr, &b, 0 );
    b = ( data != 0 ) ? ( b | ( 1 << bitNum ) ): ( b & ~( 1 << bitNum ) );
    return I2C_writeByte( devAddr, regAddr, b );
}


bool I2C_writeBits( uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data )
{
    uint8_t b;

    if( I2C_readByte( devAddr, regAddr, &b, 0 ) != 0 )
    {
        uint8_t mask = ( ( 1 << length ) - 1 ) << ( bitStart - length + 1 );

        data <<= ( bitStart - length + 1 );
        data &= mask;
        b &= ~( mask );
        b |= data;
        return I2C_writeByte( devAddr, regAddr, b );
    }
    else
    {
        return false;
    }


}


bool I2C_writeWords( uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t * data )
{
    return false;
}


bool I2C_writeWord( uint8_t devAddr, uint8_t regAddr, uint16_t data )
{
    return I2C_writeWords( devAddr, regAddr, 1, &data );
}


bool I2C_writeBitW( uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data )
{
    uint16_t w;

    I2C_readWord( devAddr, regAddr, &w, 0 );
    w = ( data != 0 ) ? ( w | ( 1 << bitNum ) ): ( w & ~( 1 << bitNum ) );

    return I2C_writeWord( devAddr, regAddr, w );
}


bool I2C_writeBitsW( uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data )
{
    return false;
}


