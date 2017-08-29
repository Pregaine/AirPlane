#include <stdio.h>
#include <string.h>
#include "NUC505Series.h"
#include "TimerManager.h"
#include "Typedef.h"
#include "E32TTL100.h"
#include "JY901.h"
#include "SD_Write.h"
#include "AirPlane.h"
#include "gpio.h"
#include "MS4525DO.h"

#define E32TTL100_RXBUFSIZE 1024
#define G_LED1 PC3_DOUT

struct
{
    TimerID tmr;

}E32TTL100;

void E32TTL100_Output_Process( void );

uint8_t E32TTL100_u8RecData[ E32TTL100_RXBUFSIZE ] = { 0 };

volatile uint32_t E32TTL100_u32comRbytes = 0;
volatile uint32_t E32TTL100_u32comRhead = 0;
volatile uint32_t E32TTL100_u32comRtail = 0;


/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void UART_TEST_HANDLE()
{
    uint8_t u8InChar = 0xFF;
    uint32_t u32IntSts = UART0->INTSTS;

    /*
     - \ref UART_INTEN_THREIEN_Msk : Tx empty interrupt
     - \ref UART_INTEN_RDAIEN_Msk  : Rx ready interrupt
    */
    // G_LED1 = ~G_LED1;
    
    if( u32IntSts & UART_INTSTS_THREINT_Msk ) 
    {     
        if( E32TTL100_u32comRbytes == 0 ) 
            return;

        // If a new word is being output
    	if( E32TTL100_u32comRbytes == E32TTL100_u32comRtail )
    	{
    		E32TTL100_u32comRhead = 0;
    	}
    	
        u8InChar = E32TTL100_u8RecData[ E32TTL100_u32comRhead ];

        UART_WRITE( UART0, u8InChar );
        
        E32TTL100_u32comRhead = ( E32TTL100_u32comRhead == ( E32TTL100_RXBUFSIZE - 1 ) ) ?0 :( E32TTL100_u32comRhead + 1 );
        E32TTL100_u32comRbytes --;
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
    // UART_EnableInt( UART0, ( UART_INTEN_THREIEN_Msk ) );

    /* Configure UART1 and set UART1 Baudrate */
    UART_Open( UART0, 115200 );

    E32TTL100.tmr = TM_CreateTimer( TIMER_ONECYCLE, _E32TTL100_TMR, NULL );
}

void E32TTl100_WriteStr( char *buf, unsigned char len )
{
    #if 0
    UART_Write( UART0, ( u8 *)buf, len );
    #endif
    
    #if 1
	// If a new word is being output
	if( E32TTL100_u32comRbytes == 0 )
	{
		E32TTL100_u32comRtail = 0;
	}

    // If buffer not empty
	if( ( E32TTL100_u32comRbytes + len ) <  E32TTL100_RXBUFSIZE ) 
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
    
    E32TTL100_u32comRhead = ( E32TTL100_u32comRhead == ( E32TTL100_RXBUFSIZE - 1 ) ) ?0 :( E32TTL100_u32comRhead + 1 );
    E32TTL100_u32comRbytes --;
}


void E32TTL100_Process( void )
{
    char str[ 64 ] = {0};
    char len;

    E32TTL100_Output_Process( );

    if( TM_GetTimerState( E32TTL100.tmr ) != TIMER_STOP )
        return;

    TM_StartTimer( E32TTL100.tmr );
    // --------------------------------------------

    #if 1
    // Angle
    sprintf( str,"%.3f,%.3f,%.3f,", stcAngle.Roll, stcAngle.Pitch, stcAngle.Yaw );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );  
    // SD_WriteStr( str, len );

    // Acc
    sprintf( str, "%.3f,%.3f,%.3f,", stcAcc.Ax, stcAcc.Ay, stcAcc.Az );
    len = strlen( str );
    E32TTl100_WriteStr( str, len ); 
    // SD_WriteStr( str, len );
 
    // Gyro
	sprintf( str, "%.3f,%.3f,%.3f,", stcGyro.Wx, stcGyro.Wy, stcGyro.Wz );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );  
    // SD_WriteStr( str, len );

    // Mag 
	sprintf( str,"%d,%d,%d,", stcMag.Hx, stcMag.Hy, stcMag.Hz );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );  
    // SD_WriteStr( str, len );

    sprintf( str, "%.3f,%.3f,%.3f,%.3f,", stcQuaternion.q0, stcQuaternion.q1, stcQuaternion.q2, stcQuaternion.q3 );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );
    // SD_WriteStr( str, len );

    // Pressure:%ld Height%.2f\r\n
    sprintf( str,"%ld,%.2f,", stcPress.lPressure, (float)stcPress.lAltitude/100 );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );
    // SD_WriteStr( str, len );
    
    // GPS
    // sprintf( str,"%.3f GPSYaw:%.1fDeg GPSV:%.3fkm/h\r\n",(float)stcGPSV.sGPSHeight/10,(float)stcGPSV.sGPSYaw/10,(float)stcGPSV.lGPSVelocity/1000);
    // Ground Speed
    sprintf( str, "%.3f,", (float)stcGPSV.lGPSVelocity/1000 );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );
    // SD_WriteStr( str, len );
    
    // Air Speed
    sprintf( str, "%.3f,", MS4525DO.airspeed );
    len = strlen( str );    
    E32TTl100_WriteStr( str, len ); 
    // SD_WriteStr( str, len );

    // Longitude Lattitude
    // sprintf( str,"Longitude:%ldDeg%.5fm Lattitude:%ldDeg%.5fm",stcLonLat.lLon/10000000,(double)(stcLonLat.lLon % 10000000)/1e5,stcLonLat.lLat/10000000,(double)(stcLonLat.lLat % 10000000)/1e5);
    sprintf( str,"%ld%.5f,%ld%.5f,",stcLonLat.lLon/10000000,(double)(stcLonLat.lLon % 10000000)/1e5,stcLonLat.lLat/10000000,(double)(stcLonLat.lLat % 10000000)/1e5);
    len = strlen( str );    
    E32TTl100_WriteStr( str, len ); 
    // SD_WriteStr( str, len );
    
    /*
    sprintf( str,"Time:20%d-%d-%d %d:%d:%d\r\n",stcTime.ucYear,stcTime.ucMonth,stcTime.ucDay,stcTime.ucHour,stcTime.ucMinute, stcTime.ucSecond );
    E32TTl100_WriteStr( str, strlen( str ) ); 
    SD_WriteStr( str, strlen( str ) );

    sprintf( str, "jy901 rx len %d\r\n", g_u32comRbytes );
    E32TTl100_WriteStr( str, strlen( str ) );
    */
    // --------------------------------------------
    #endif

    # if 1
    sprintf( str, "%.3f\r\n", AirPlane_z_force );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );
    // SD_WriteStr( str, len );
    # endif

    #if 0    
    sprintf( str, "%.3f,%.3f,%.3f,%.3f\r\n", stcQuaternion.q0, stcQuaternion.q1, stcQuaternion.q2, stcQuaternion.q3 );
    len = strlen( str );
    E32TTl100_WriteStr( str, len );

    // Acc
    sprintf( str, "%.3f,%.3f,%.3f\r\n", stcAcc.Ax, stcAcc.Ay, stcAcc.Az );
    E32TTl100_WriteStr( str, strlen( str ) ); 
    #endif
    
}