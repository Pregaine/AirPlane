#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "NUC505Series.h"
#include "TimerManager.h"
#include "Typedef.h"
#include "E32TTL100.h"
#include "JY901.h"
#include "SD_Write.h"
#include "AirPlane.h"
#include "gpio.h"
#include "MS4525DO.h"

#define _E32TTL100_RXBUFSIZE 128
#define G_LED1 PC3_DOUT

#define _PID_BUFSIZE 64


struct
{
    TimerID tmr;

}E32TTL100;

void E32TTL100_Output_Process( void );

uint8_t E32TTL100_u8RecData[ _E32TTL100_RXBUFSIZE ] = { 0 };

volatile uint32_t E32TTL100_u32comRbytes = 0;
volatile uint32_t E32TTL100_u32comRhead = 0;
volatile uint32_t E32TTL100_u32comRtail = 0;

uint8_t PID_RecData[ _PID_BUFSIZE ] = { 0 };

volatile uint32_t PID_comRbytes = 0;
volatile uint32_t PID_comRhead = 0;
volatile uint32_t PID_comRtail = 0;


/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void UART_TEST_HANDLE()
{
    /*
     - \ref UART_INTEN_THREIEN_Msk : Tx empty interrupt
     - \ref UART_INTEN_RDAIEN_Msk  : Rx ready interrupt
    */
    
    uint8_t 		u8InChar = 0xFF;
	uint32_t		u32IntSts = UART0->INTSTS;

    G_LED1 = ~G_LED1;

    // Rx ready interrupt
	if( u32IntSts & UART_INTSTS_RDAINT_Msk )
	{
		/* Get all the input characters */
		while( UART_IS_RX_READY( UART0 ) )
		{
			/* Get the character from UART Buffer */
			u8InChar = UART_READ( UART0 );

			/* Check if buffer full */
			if( PID_comRbytes < _PID_BUFSIZE )
			{
				/* Enqueue the character */
				PID_RecData[ PID_comRtail ] = u8InChar;
				PID_comRtail = ( PID_comRtail == ( _PID_BUFSIZE - 1 ) ) ? 0: ( PID_comRtail + 1 );
				PID_comRbytes ++;
			}
			else 
			{
                printf( "E32TTL100 Rx Overflow");
			}
		}
	}
}


/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 0 interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_IRQHandler(void)
{
    UART_TEST_HANDLE();
}


void E32TTL100_Init( void )
{
    /* Enable IP clock */
    CLK_EnableModuleClock( UART0_MODULE );

    /* Select IP clock source */
    CLK_SetModuleClock( UART0_MODULE, CLK_UART0_SRC_EXT, 0 );

    //--- Set GPB0/1 multi-function pins for UART0 RXD and TXD
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB0MFP_Msk)) | SYS_GPB_MFPL_PB0MFP_UART0_TXD;
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk)) | SYS_GPB_MFPL_PB1MFP_UART0_RXD;
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART0 module */
    SYS_ResetModule( UART0_RST );

    /* Enable Receive Data Available Interrupt Enable Control  */
    UART_EnableInt( UART0, ( UART_INTEN_RDAIEN_Msk ) );

    /* Configure UART1 and set UART1 Baudrate */
    UART_Open( UART0, 115200 );

    E32TTL100.tmr = TM_CreateTimer( TIMER_ONECYCLE, _E32TTL100_TMR, NULL );
}

void E32TTl100_WriteStr( char *buf )
{
    u8 i = 0, len;
    #if 0
    UART_Write( UART0, ( u8 *)buf, len );
    #endif

    for( ; buf[ i ] != 0; i ++ )

    len = i + 1;    
    
    #if 1
	// If a new word is being output
	if( E32TTL100_u32comRbytes == 0 )
	{
		E32TTL100_u32comRtail = 0;
	}

    // If buffer not empty
	if( ( E32TTL100_u32comRbytes + len ) <  _E32TTL100_RXBUFSIZE ) 
	{
		memcpy( &E32TTL100_u8RecData[ E32TTL100_u32comRtail ], buf, len );

		E32TTL100_u32comRtail += len;
		E32TTL100_u32comRbytes += len;
	}
	else 
	{
	    // Set the array size to 0
		// buf overflow	
	}
	#endif
}

u8 E32TTL100_GetData( u8 *data )
{

    if( PID_comRbytes == 0 ) 
        return 0;
    
	// If a new word is being output
	if( PID_comRbytes == PID_comRtail )
	{
		PID_comRhead = 0;
	}
    
    *data = PID_RecData[ PID_comRhead ];
    PID_comRhead = ( PID_comRhead == ( _PID_BUFSIZE - 1 ) ) ?0: ( PID_comRhead + 1 );
    PID_comRbytes --;

    return 1;
}


/*----------------------------------------------*/
/* Get a line from the input                    */
/*----------------------------------------------*/
void E32TTL100_Get_Line( )
{
    static u8 index = 0;
    static char buf[ 64 ];
    u8 data;

    char *pEnd;
    u8 i;

    if( E32TTL100_GetData( &data ) == 0 )
        return;

    if( index > sizeof( buf ) )
        index = 0;

    if( ( data >= '0' && data <= '9' ) || data == '.' || data == ' ' || data == '\r' || data == '-' )
    {
        buf[ index ++ ] = data;

        if( buf[ index - 1 ] == '\r' )
        {
            buf[ index - 1 ] = 0;
            
            PID_Pitch.Kp = strtof ( buf, &pEnd );
            PID_Pitch.Ki = strtof ( pEnd, &pEnd );
            PID_Pitch.Kd = strtof ( pEnd, &pEnd );

            PID_Row.Kp = strtof ( pEnd, &pEnd );
            PID_Row.Ki = strtof ( pEnd, &pEnd );
            PID_Row.Kd = strtof ( pEnd, &pEnd );

            PID_Yaw.Kp = strtof ( pEnd, &pEnd );
            PID_Yaw.Ki = strtof ( pEnd, &pEnd );
            PID_Yaw.Kd = strtof ( pEnd, NULL );

            index = 0;

            #if 0
            printf( "\r\nPID Pitch:%.3f, %.3f, %.3f", PID_Pitch.Kp, PID_Pitch.Ki, PID_Pitch.Kd );
            printf( "\r\nPID Row:%.3f, %.3f, %.3f", PID_Row.Kp, PID_Row.Ki, PID_Row.Kd );
            printf( "\r\nPID Yaw:%.3f, %.3f, %.3f", PID_Yaw.Kp, PID_Yaw.Ki, PID_Yaw.Kd );
            #endif
            
            for( i = 0; i < sizeof( buf ); i ++ )
                buf[ i ] = 0;
        }
    }
    else
    {   
        printf( "fail len index:%d\n", index );

        index = 0;

        for( i = 0; i < sizeof( buf ); i ++ )
            buf[ i ] = 0;
    }

    
}


void E32TTL100_Output_Process( void )
{
    uint8_t u8InChar = 0xFF;

    if( E32TTL100_u32comRbytes == 0 ) 
        return;

    // If a new word is being output
	if( E32TTL100_u32comRbytes == E32TTL100_u32comRtail )
	{
		E32TTL100_u32comRhead = 0;
	}
	
    u8InChar = E32TTL100_u8RecData[ E32TTL100_u32comRhead ];

    UART_Write( UART0, &u8InChar, 1 );
    
    E32TTL100_u32comRhead = ( E32TTL100_u32comRhead == ( _E32TTL100_RXBUFSIZE - 1 ) ) ?0 :( E32TTL100_u32comRhead + 1 );
    E32TTL100_u32comRbytes --;
}


void E32TTL100_Process( void )
{
    char str[ 64 ];

    E32TTL100_Output_Process( );

    E32TTL100_Get_Line( );

    if( TM_GetTimerState( E32TTL100.tmr ) != TIMER_STOP )
        return;

    TM_StartTimer( E32TTL100.tmr );
    // --------------------------------------------

    #if 1
    // Angle
    sprintf( str,"%.2f,%.2f,%.2f", stcAngle.Roll, stcAngle.Pitch, stcAngle.Yaw );
    E32TTl100_WriteStr( str );  
    // SD_WriteStr( str );
    #endif

    #if 0
    // Acc
    sprintf( str, ",%.2f,%.2f,%.2f", stcAcc.Ax, stcAcc.Ay, stcAcc.Az );
    E32TTl100_WriteStr( str ); 
    // SD_WriteStr( str );
 
    // Gyro
	sprintf( str, ",%.2f,%.2f,%.2f", stcGyro.Wx, stcGyro.Wy, stcGyro.Wz );
    E32TTl100_WriteStr( str );  
    // SD_WriteStr( str );

    // Mag 
	sprintf( str,",%d,%d,%d", stcMag.Hx, stcMag.Hy, stcMag.Hz );
    E32TTl100_WriteStr( str );  
    // SD_WriteStr( str );

    sprintf( str, ",%.2f,%.2f,%.2f,%.2f", stcQuaternion.q0, stcQuaternion.q1, stcQuaternion.q2, stcQuaternion.q3 );
    E32TTl100_WriteStr( str );
    // SD_WriteStr( str );

    // Pressure:%ld Height%.2f\r\n
    sprintf( str,",%ld,%.2f", stcPress.lPressure, (float)stcPress.lAltitude/100 );
    E32TTl100_WriteStr( str );
    // SD_WriteStr( str );
    
    // GPS
    // sprintf( str,"%.3f GPSYaw:%.1fDeg GPSV:%.3fkm/h\r\n",(float)stcGPSV.sGPSHeight/10,(float)stcGPSV.sGPSYaw/10,(float)stcGPSV.lGPSVelocity/1000);
    // Ground Speed
    sprintf( str, ",%.2f", (float)stcGPSV.lGPSVelocity/1000 );
    E32TTl100_WriteStr( str );
    // SD_WriteStr( str );
    
    // Air Speed
    sprintf( str, ",%.2f", MS4525DO.airspeed );    
    E32TTl100_WriteStr( str ); 
    // SD_WriteStr( str );

    // Longitude Lattitude
    // sprintf( str,"Longitude:%ldDeg%.5fm Lattitude:%ldDeg%.5fm",stcLonLat.lLon/10000000,(double)(stcLonLat.lLon % 10000000)/1e5,stcLonLat.lLat/10000000,(double)(stcLonLat.lLat % 10000000)/1e5);
    sprintf( str,",%ld%.5f,%ld%.5f",stcLonLat.lLon/10000000,(double)(stcLonLat.lLon % 10000000)/1e5,stcLonLat.lLat/10000000,(double)(stcLonLat.lLat % 10000000)/1e5);   
    E32TTl100_WriteStr( str ); 
    // SD_WriteStr( str );
    
    /*
    sprintf( str,"Time:20%d-%d-%d %d:%d:%d\r\n",stcTime.ucYear,stcTime.ucMonth,stcTime.ucDay,stcTime.ucHour,stcTime.ucMinute, stcTime.ucSecond );
    E32TTl100_WriteStr( str ); 
    SD_WriteStr( str );

    sprintf( str, "jy901 rx len %d\r\n", g_u32comRbytes );
    E32TTl100_WriteStr( str );
    */
    // --------------------------------------------
    #endif

    # if 1
    sprintf( str, ",%.2f", AirPlane_z_force );
    E32TTl100_WriteStr( str );
    // SD_WriteStr( str );
    # endif

    #if 1
    sprintf( str, ",%.1f,%.1f,%.1f", PID_Pitch.Kp, PID_Pitch.Ki, PID_Pitch.Kd );
    E32TTl100_WriteStr( str );

    sprintf( str, ",%.1f,%.1f,%.1f", PID_Row.Kp, PID_Row.Ki, PID_Row.Kd );
    E32TTl100_WriteStr( str );

    sprintf( str, ",%.1f,%.1f,%.1f\r\n", PID_Yaw.Kp, PID_Yaw.Ki, PID_Yaw.Kd );
    E32TTl100_WriteStr( str );
    #endif

    #if 0    
    sprintf( str, "%.3f,%.3f,%.3f,%.3f\r\n", stcQuaternion.q0, stcQuaternion.q1, stcQuaternion.q2, stcQuaternion.q3 );
    E32TTl100_WriteStr( str );

    // Acc
    sprintf( str, "%.3f,%.3f,%.3f\r\n", stcAcc.Ax, stcAcc.Ay, stcAcc.Az );
    E32TTl100_WriteStr( str ); 
    #endif
    
}
