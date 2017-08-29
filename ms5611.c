

/*
MS5611-01BA.cpp - Interfaces a Measurement Specialities MS5611-01BA with Arduino
See http://www.meas-spec.com/downloads/MS5611-01BA01.pdf for the device datasheet

Copyright (C) 2011 Fabio Varesano <fvaresano@yahoo.it>
Adapted to Cortex-M4 Fly Controller by Nuvoton

Development of this code has been supported by the Department of Computer Science,
Universita' degli Studi di Torino, Italy within the Piemonte Project
http://www.piemonte.di.unito.it/


This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/*============================================================================*
 * O     O          __                   ______  __                           *
 *  \   /      /\  / /_      _    __    / /___/ / /_     _                    *
 *   [+]      /  \/ / \\    //__ / /__ / /____ / / \\   //                    *
 *  /   \    / /\  /   \\__// --/ /---/ /----// /   \\_//                     *
 * O     O  /_/  \/     \__/    \_\/ /_/     /_/ ____/_/                      *
 *                                                                            *
 *                                                                            *
 * Multi-Rotor controller firmware for Nuvoton Cortex M4 series               *
 *                                                                            *
 * Written by by T.L. Shen for Nuvoton Technology.                            *
 * tlshen@nuvoton.com/tzulan611126@gmail.com                                  *
 *                                                                            *
 *============================================================================*
 */
#include <stdint.h>
#include "NUC505Series.h"
#include "NUC505_Timer.h"
#include "Typedef.h"
#include "stdio.h"
#include "ms5611.h"
#include "I2CDev.h"
#include "math.h"

#define EXTRA_PRECISION         5 
#define CONVERSION_TIME_MS      10 
#define PRESSURE_PER_TEMP       7 
#define FIX_TEMP                25        
#define I2CDEV_NO_MEM_ADDR      0xFF

typedef struct 
{
uint16_t psens;
uint16_t off;
uint16_t tcs;
uint16_t tco;
uint16_t tref;
uint16_t tsens;
} CalReg;


static uint8_t devAddr;
static bool isInit;

static CalReg calReg;
static uint32_t lastPresConv;
static uint32_t lastTempConv;
static int32_t tempCache;

static uint8_t readState = 0;
static uint32_t lastConv = 0;
static int32_t tempDeltaT;


bool ms5611Init()
{
    if( isInit )
    {
        return TRUE;

    }

    devAddr = MS5611_ADDR_CSB_LOW;

    ms5611Reset();

    // DelayMsec(5);
    if( ms5611ReadPROM() == FALSE )
    {
        return FALSE;
    }

    isInit = TRUE;

    return TRUE;
}


bool ms5611SelfTest( void )
{
    bool testStatus = TRUE;
    int32_t rawPress;
    int32_t rawTemp;
    int32_t deltaT;
    float pressure;
    float temperature;

    if( !isInit )
    {
        return FALSE;

    }

    ms5611StartConversion( MS5611_D1 + MS5611_OSR_4096 );

    // DelayMsec(CONVERSION_TIME_MS);
    rawPress = ms5611GetConversion( MS5611_D1 + MS5611_OSR_4096 );

    ms5611StartConversion( MS5611_D2 + MS5611_OSR_4096 );

    // DelayMsec(CONVERSION_TIME_MS);
    rawTemp = ms5611GetConversion( MS5611_D2 + MS5611_OSR_4096 );

    deltaT = ms5611CalcDeltaTemp( rawTemp );
    temperature = ms5611CalcTemp( deltaT );
    pressure = ms5611CalcPressure( rawPress, deltaT );

    if( ms5611EvaluateSelfTest( MS5611_ST_PRESS_MIN, MS5611_ST_PRESS_MAX, pressure, "pressure" ) &&
         ms5611EvaluateSelfTest ( MS5611_ST_TEMP_MIN, MS5611_ST_TEMP_MAX, temperature, "temperature" ) )
    {
        //DEBUG_PRINT("Self test [OK].\n");
        printf( "Ground Altitude:%f\n", ms5611PressureToAltitude( &pressure ) );
    }
    else
    {
        testStatus = FALSE;
    }

    return testStatus;
}


bool ms5611EvaluateSelfTest( float min, float max, float value, char * string )
{
    if( value < min || value > max )
    {
        printf( "Self test %s [FAIL]. low: %0.2f, high: %0.2f, measured: %0.2f\n", 
            string, min, max, value );
        return FALSE;
    }

    return TRUE;
}


float ms5611GetPressure( uint8_t osr )
{
    // see datasheet page 7 for formulas
    int64_t off, sens;
    int32_t rawPress = ms5611RawPressure( osr );
    int64_t dT = ( int64_t )

    ms5611GetDeltaTemp( osr );

    if( dT == 0 )
    {
        return 0;
    }

    off = ( ( ( int64_t ) calReg.off ) << 16 ) + ( ( calReg.tco * dT ) >> 7 );
    sens = ( ( ( int64_t ) calReg.psens ) << 15 ) + ( ( calReg.tcs * dT ) >> 8 );

    if( rawPress != 0 )
    {
        return ( ( ( ( rawPress * sens ) >> 21 ) -off ) >> ( 15 - EXTRA_PRECISION ) ) / ( ( 1 << EXTRA_PRECISION ) * 100.0 );
    }
    else
    {
        return 0;
    }
}


float ms5611CalcPressure( int32_t rawPress, int32_t dT )
{
    int64_t off;
    int64_t sens;

    if( rawPress == 0 || dT == 0 )
    {
        return 0;
    }

    off = ( ( ( int64_t ) calReg.off ) << 16 ) + ( ( calReg.tco * ( int64_t ) dT ) >> 7 );
    sens = ( ( ( int64_t ) calReg.psens ) << 15 ) + ( ( calReg.tcs * ( int64_t ) dT ) >> 8 );

    return ( ( ( ( rawPress * sens ) >> 21 ) -off ) >> ( 15 - EXTRA_PRECISION ) ) / ( ( 1 << EXTRA_PRECISION ) * 100.0 );
}


float ms5611GetTemperature( uint8_t osr )
{
    // see datasheet page 7 for formulas
    int32_t dT;

    dT = ms5611GetDeltaTemp( osr );

    if( dT != 0 )
    {
        return ms5611CalcTemp( dT );
    }
    else
    {
        return 0;
    }
}


int32_t ms5611GetDeltaTemp( uint8_t osr )
{
    int32_t rawTemp = ms5611RawTemperature( osr );

    if( rawTemp != 0 )
    {
        return ms5611CalcDeltaTemp( rawTemp );
    }
    else
    {
        return 0;
    }
}


float ms5611CalcTemp( int32_t deltaT )
{
    if( deltaT == 0 )
    {
        return 0;
    }
    else
    {
        return ( float ) ( ( ( 1 << EXTRA_PRECISION ) * 2000 ) + ( ( ( int64_t ) deltaT * calReg.tsens ) >> ( 23 - EXTRA_PRECISION ) ) ) / ( ( 1 << EXTRA_PRECISION ) * 100.0 );
    }
}


int32_t ms5611CalcDeltaTemp( int32_t rawTemp )
{
    if( rawTemp == 0 )
    {
        return 0;
    }
    else
    {
        return rawTemp - ( ( ( int32_t ) calReg.tref ) << 8 );
    }
}


int32_t ms5611RawPressure( uint8_t osr )
{
    uint32_t now = system_Tick;

    if( lastPresConv != 0 && ( now - lastPresConv ) >= CONVERSION_TIME_MS )
    {
        lastPresConv = 0;
        return ms5611GetConversion( MS5611_D1 + osr );
    }
    else
    {
        if( lastPresConv == 0 && lastTempConv == 0 )
        {
            ms5611StartConversion( MS5611_D1 + osr );
            lastPresConv = now;
        }

        return 0;
    }
}


int32_t ms5611RawTemperature( uint8_t osr )
{
    uint32_t now = system_Tick;

    if( lastTempConv != 0 && ( now - lastTempConv ) >= CONVERSION_TIME_MS )
    {
        lastTempConv = 0;
        tempCache = ms5611GetConversion( MS5611_D2 + osr );
        return tempCache;
    }
    else
    {
        if( lastTempConv == 0 && lastPresConv == 0 )
        {
            ms5611StartConversion( MS5611_D2 + osr );
            lastTempConv = now;
        }

        return tempCache;
    }
}


// see page 11 of the datasheet
void ms5611StartConversion( uint8_t command )
{
    I2C_writeBytes( devAddr, command, 0, 0 );
}


int32_t ms5611GetConversion( uint8_t command )
{
    int32_t conversion = 0;
    uint8_t buffer[MS5611_D1D2_SIZE];

    I2C_readBytes( devAddr, 0, MS5611_D1D2_SIZE, buffer, 0 );

    conversion = ( ( int32_t ) buffer[0] << 16 ) | ( ( int32_t ) buffer[1] << 8 ) | buffer[2];

    return conversion;
}


bool ms5611ReadPROM()
{
    uint8_t buffer[MS5611_PROM_REG_SIZE];
    uint16_t * pCalRegU16 = ( uint16_t * ) &calReg;
    int32_t i = 0;
    bool status = FALSE;

    for( i = 0; i < MS5611_PROM_REG_COUNT; i++ )
    {
        status = I2C_readBytes( devAddr, MS5611_PROM_BASE_ADDR + ( i * MS5611_PROM_REG_SIZE ), MS5611_PROM_REG_SIZE,
             buffer, 1 );
        pCalRegU16[i] = ( ( uint16_t ) buffer[0] << 8 ) | buffer[1];
        printf( "Baro Rom[%d]:0x%x:\n", i, pCalRegU16[i] );
    }

    return status;
}


void ms5611Reset()
{
    I2C_writeBytes( devAddr, MS5611_RESET, 0, 0 );
}


bool ms5611GetData( float * pressure, float * temperature, float * asl )
{
    int32_t tempPressureRaw, tempTemperatureRaw;

    uint32_t now = system_Tick;

    if( ( now - lastConv ) < CONVERSION_TIME_MS )
    {
        return false;
    }

    lastConv = now;

    if( readState == 0 )
    {
        // read temp
        ++readState;
        tempTemperatureRaw = ms5611GetConversion( MS5611_D2 + MS5611_OSR_DEFAULT );
        tempDeltaT = ms5611CalcDeltaTemp( tempTemperatureRaw );
        *temperature = ms5611CalcTemp( tempDeltaT );

        // cmd to read pressure
        ms5611StartConversion( MS5611_D1 + MS5611_OSR_DEFAULT );
        return false;
    }
    else
    {
        // read pressure
        ++readState;
        tempPressureRaw = ms5611GetConversion( MS5611_D1 + MS5611_OSR_DEFAULT );
        *pressure = ms5611CalcPressure( tempPressureRaw, tempDeltaT );
        *asl = 0;                                   //ms5611PressureToAltitude(pressure);

        if( readState == PRESSURE_PER_TEMP )
        {
            // cmd to read temp
            ms5611StartConversion( MS5611_D2 + MS5611_OSR_DEFAULT );
            readState = 0;
        }
        else
        {
            // cmd to read pressure
            ms5611StartConversion( MS5611_D1 + MS5611_OSR_DEFAULT );
        }

        return true;
    }
}


float ms5611PressureToAltitude( float * pressure /*, float* ground_pressure, float* ground_temp*/ )
{
    if( *pressure > 0 )
    {
        return ( ( pow( ( 1015.7f / *pressure ), CONST_PF ) - 1.0f ) * ( FIX_TEMP + 273.15f ) ) / 0.0065f;
    }
    else
    {
        return 0;
    }
}


