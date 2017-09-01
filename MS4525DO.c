#include <stdio.h>
#include "NUC505Series.h"
#include "TimerManager.h"
#include "NUC505_I2c.h"
#include "MS4525DO.h"
#include "I2CDev.h"
#include "Typedef.h"

_MS4525DO MS4525DO;

void MS4525DO_Read_DF4( void );

void MS4525DO_Init( void )
{
    MS4525DO.rawpressure    = 0x0;
    MS4525DO.rawtemperature = 0x0;
    MS4525DO.status         = _MS4525DO_Busy;
    MS4525DO.temperature    = 0;
    MS4525DO.airspeed       = 0;
    
    MS4525DO.tmr = TM_CreateTimer( TIMER_ONECYCLE, _MS4525DO_TMR, NULL );
}

// Read Temperature, diff pressure
void MS4525DO_Read_DF4( void )
{
    u8 buf[ 4 ];
    u16 val;

    MS4525DO.status =  0xFF;

    I2C_readBytes( _MS4525DO_Addr, 0, 4, buf, 1 );

    /*
    printf( "0x%x\n", buf[ 0 ] >> 6 );

    printf( "0x%x, 0x%x, 0x%x, 0x%x\n", buf[ 0 ] & 0x3F, buf[ 1 ], buf[ 2 ], buf[ 3 ] );
    */

    MS4525DO.status = buf[ 0 ] >> 6;

    if( MS4525DO.status == _MS4525DO_Ok )
    {
        val = ( u16 )( buf[ 0 ] & 0x3F ) << 8;
        
        MS4525DO.rawpressure =  val | buf[ 1 ];
        MS4525DO.rawtemperature = ( ( ( u16 ) buf[ 2 ] ) << 3 ) | ( buf[ 3 ] >> 5 );
        
        MS4525DO.temperature = ( ( float ) MS4525DO.rawtemperature * 0.09770395701f ) - 50;

        // printf( "MS4525DO.rawpressure:%d\r\n", MS4525DO.rawpressure );
        // printf( "MS4525DO.temperature:%.2f\r\n", MS4525DO.temperature );
    }
    else 
    {
        printf( "MS4520 Get Data Fail\r\n" );
    }
}

void MS4525DO_Process()
{
    if( TM_GetTimerState( MS4525DO.tmr ) != TIMER_STOP )
        return;

    TM_StartTimer( MS4525DO.tmr );        
    
    MS4525DO_Read_DF4( );    
}


