

/*****************************************************************************
 *			Copyright c E-LEAD Electronics All Rights Reserved
 *****************************************************************************			
 * Description:
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "NUC505Series.h"
#include "TimerManager.h"
#include "Typedef.h"
#include "JY901.h"
#include <stdio.h>
#include <string.h>
#include "gpio.h"

#define RXBUFSIZE 1024
#define G_LED1 PC3_DOUT

u8 const Set_ReqOut_Cmd[ ] = { 0xFF, 0xAA, 0x02, 0xFF, 0x07 };
u8 const Set_ReqUpdateFreq_Cmd[ ] = { 0xFF, 0xAA, 0x03, _RATE_200HZ, 0x00 };
u8 const Set_D1PortMode_Cmd[ ] = { 0xFF, 0xAA, 0x0F, 0x05, 0x00 };


/*---------------------------------------------------------------------------------------------------------*/
/* Global variables 																					   */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t g_u8RecData[ RXBUFSIZE ] = { 0 };
uint8_t JY901_Buf[ 9 ] = { 0 };

volatile uint32_t g_u32comRbytes = 0;
volatile uint32_t g_u32comRhead = 0;
volatile uint32_t g_u32comRtail = 0;

_STime stcTime;
_SAcc stcAcc;
_SGyro stcGyro;
_SAngle stcAngle;
_SMag stcMag;
_SDStatus stcDStatus;
_SPress stcPress;
_SLonLat stcLonLat;
_SGPSV stcGPSV;
_SQuaternion stcQuaternion;


void UART1_TEST_HANDLE( void );

void JY901_Init()
{
	/* Enable IP clock */
	CLK_EnableModuleClock( UART1_MODULE );

	/* Select IP clock source */
	CLK_SetModuleClock( UART1_MODULE, CLK_UART1_SRC_EXT, 0 );
 
	//--- Set GPB6/7 multi-function pins for UART1 RXD and TXD
	                                   
	SYS->GPA_MFPH = ( SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA8MFP_Msk)) | SYS_GPA_MFPH_PA8MFP_UART1_TXD;
	SYS->GPA_MFPH = ( SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA9MFP_Msk)) | SYS_GPA_MFPH_PA9MFP_UART1_RXD;

	/*---------------------------------------------------------------------------------------------------------*/
	/* Init UART																							   */
	/*---------------------------------------------------------------------------------------------------------*/
	/* Reset UART1 module */
	SYS_ResetModule( UART1_RST );

	/* Configure UART1 and set UART1 Baudrate */
	UART_Open( UART1, 115200 );
	
	/* Enable Receive Data Available Interrupt Enable Control  */
    // UART_INTEN_THREIEN_Msk
    UART_EnableInt( UART1, ( UART_INTEN_RDAIEN_Msk ) );

    // Init cmd 200Hz
	UART_Write( UART1, ( u8 *)Set_ReqUpdateFreq_Cmd, sizeof( Set_ReqUpdateFreq_Cmd ) );

	// Init Request Output Cmd
	UART_Write( UART1, ( u8 *)Set_ReqOut_Cmd, sizeof( Set_ReqOut_Cmd ) );

	UART_Write( UART1, ( u8 *)Set_D1PortMode_Cmd, sizeof( Set_D1PortMode_Cmd ) );
}


/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 1 interrupt event 														   */
/*---------------------------------------------------------------------------------------------------------*/
void UART1_IRQHandler(void)
{
	UART1_TEST_HANDLE();
}


/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function																				   */
/*---------------------------------------------------------------------------------------------------------*/
void UART1_TEST_HANDLE()
{
    /*
     - \ref UART_INTEN_THREIEN_Msk    : Tx empty interrupt
     - \ref UART_INTEN_RDAIEN_Msk     : Rx ready interrupt
    */

	uint8_t 		u8InChar = 0xFF;
	uint32_t		u32IntSts = UART1->INTSTS;

    // Rx ready interrupt
	if( u32IntSts & UART_INTSTS_RDAINT_Msk )
	{
		/* Get all the input characters */
		while( UART_IS_RX_READY( UART1 ) )
		{
			/* Get the character from UART Buffer */
			u8InChar = UART_READ( UART1 );

			/* Check if buffer full */
			if( g_u32comRbytes < RXBUFSIZE )
			{
				/* Enqueue the character */
				g_u8RecData[ g_u32comRtail ] = u8InChar;
				g_u32comRtail = ( g_u32comRtail == ( RXBUFSIZE - 1 ) ) ? 0: ( g_u32comRtail + 1 );
				g_u32comRbytes ++;
			}
			else 
			{
                printf( "JY901 Rx Overflow");
			}
		}
	}
}

float JY901_Combine( u8 *hibyte, u8 *lobyte )
{
    short val;

    val = ( ( short )( *hibyte ) << 8 ) | ( *lobyte );
    val = ( *hibyte & 0x80 ) ?( val - 65536 ) :val;

    return ( float )val;
}



u8 JY901_GetData( u8 *data )
{

    if( g_u32comRbytes == 0 ) 
        return 0;
    
	// If a new word is being output
	if( g_u32comRbytes == g_u32comRtail )
	{
		g_u32comRhead = 0;
	}
    
    *data = g_u8RecData[ g_u32comRhead ];
    g_u32comRhead = ( g_u32comRhead == ( RXBUFSIZE - 1 ) ) ?0: ( g_u32comRhead + 1 );
    g_u32comRbytes --;

    return 1;
}

u8 JY901_Decode( )
{
    // u8 i;
    u8 buf;
    static u8 index = 0;
    static u8 chksum = 0;

    if( JY901_GetData( &buf ) == 0 )
        return 0;

    /*
    printf( "0x%x", index );
    printf( "\r\n" );
    */

    if( index == 0 )
    {
        if( buf != 0x55 )
        {
            return 0;
        }

        index ++;
        chksum = 0x55;
    }
    else if( index == 1 )
    {
        if( buf < 0x50 || buf > 0x5A )
        {
            index = 0;
            return 0;
        }

        index ++;
        chksum += buf;
        JY901_Buf[ 0 ] = buf;
    }
    else if( index < 10 )
    {
        JY901_Buf[ index - 1 ] = buf;

        chksum += buf;
        index ++;

        return 0;
    }
    else if( index == 10 )
    {
        if( buf != chksum )
        {
            index = 0;
            return 0;
        }

        /*
        for( i = 0; i < 9; i ++ )      
            printf( "0x%x", JY901_Buf[ i ] );

        printf( "\r\n" );
        */
        
        return 1;
    }

    return 0;
    
}


void JY901_Process( )
{
    uint32_t cnt = g_u32comRbytes;
    u8 result = 0;

    if( cnt == 0 ) return;

    cnt = ( cnt > 10 ) ?10 :cnt;

    while( cnt > 0 && result == 0 )
    {
        result = JY901_Decode();
        
        cnt --; 
    }

    if( result == 0 ) return;

    switch( JY901_Buf[ 0 ] )
    {
        case 0x50:  
            // memcpy( &stcTime, &JY901_Buf[1], 8 ); 
            stcTime.ucYear   = JY901_Buf[ 1 ];
            stcTime.ucMonth  = JY901_Buf[ 2 ];
            stcTime.ucDay    = JY901_Buf[ 3 ];
            stcTime.ucHour   = JY901_Buf[ 4 ];
            stcTime.ucSecond = JY901_Buf[ 5 ];
            break;

        case 0x51:  
            // memcpy( &stcAcc, &JY901_Buf[ 1 ],8 );

            // ax =((AxH<<8)|AxL)/32768*16g(g 为重力加速度，可取 9.8m/s2) 
            // ay =((AyH<<8)|AyL)/32768*16g(g 为重力加速度，可取 9.8m/s2) 
            // az =((AzH<<8)|AzL)/32768*16g(g 为重力加速度，可取 9.8m/s2) 
            stcAcc.AxL = JY901_Buf[ 1 ];
            stcAcc.AxH = JY901_Buf[ 2 ];

            stcAcc.AyL = JY901_Buf[ 3 ];
            stcAcc.AyH = JY901_Buf[ 4 ];

            stcAcc.AzL = JY901_Buf[ 5 ];
            stcAcc.AzH = JY901_Buf[ 6 ];

            stcAcc.Ax = ( JY901_Combine( &stcAcc.AxH, &stcAcc.AxL ) / 32768 ) * 16;
            stcAcc.Ay = ( JY901_Combine( &stcAcc.AyH, &stcAcc.AyL ) / 32768 ) * 16;
            stcAcc.Az = ( JY901_Combine( &stcAcc.AzH, &stcAcc.AzL ) / 32768 ) * 16;

            break;

        case 0x52:  
            // memcpy( &stcGyro, &JY901_Buf[ 1 ], 8 );
            // wx =((wxH<<8)|wxL)/32768*2000(°/s) 
            // wy =((wyH<<8)|wyL)/32768*2000(°/s) 
            // wz =((wzH<<8)|wzL)/32768*2000(°/s) 
            stcGyro.WxL = JY901_Buf[ 1 ];
            stcGyro.WxH = JY901_Buf[ 2 ];

            stcGyro.WyL = JY901_Buf[ 3 ];
            stcGyro.WyH = JY901_Buf[ 4 ];

            stcGyro.WzL = JY901_Buf[ 5 ];
            stcGyro.WzH = JY901_Buf[ 6 ];

            stcGyro.Wx = ( JY901_Combine( &stcGyro.WxH, &stcGyro.WxL ) / 32768 ) * 2000;
            stcGyro.Wy = ( JY901_Combine( &stcGyro.WyH, &stcGyro.WyL ) / 32768 ) * 2000;
            stcGyro.Wz = ( JY901_Combine( &stcGyro.WzH, &stcGyro.WzL ) / 32768 ) * 2000;
            break;

        case 0x53:  
            stcAngle.RollL  = JY901_Buf[ 1 ];
            stcAngle.RollH  = JY901_Buf[ 2 ];
            stcAngle.PitchL = JY901_Buf[ 3 ];
            stcAngle.PitchH = JY901_Buf[ 4 ];
            stcAngle.YawL   = JY901_Buf[ 5 ];
            stcAngle.YawH   = JY901_Buf[ 6 ];
            stcAngle.TL     = JY901_Buf[ 7 ];
            stcAngle.TH     = JY901_Buf[ 8 ];      

            // 滚转角（x 轴）Roll=((RollH<<8)|RollL)/32768*180(°) 
            // 俯仰角（y 轴）Pitch=((PitchH<<8)|PitchL)/32768*180(°)
            // 偏航角（z 轴）Yaw=((YawH<<8)|YawL)/32768*180(°) 
 
            stcAngle.Roll  = JY901_Combine( &stcAngle.RollH, &stcAngle.RollL ) / 32768 * 180;
            stcAngle.Pitch = JY901_Combine( &stcAngle.PitchH, &stcAngle.PitchL ) / 32768 * 180;
            stcAngle.Yaw   = JY901_Combine( &stcAngle.YawH, &stcAngle.YawL ) / 32768 * 180;
            G_LED1 = ~G_LED1;
            
            break;
        case 0x54:  
            // memcpy( &stcMag, &JY901_Buf[ 1 ], 8 );
            // 磁场（x 轴）Hx=(( HxH<<8)| HxL) 
            // 磁场（y 轴）Hy=(( HyH <<8)| HyL) 
            // 磁场（z 轴）Hz =(( HzH<<8)| HzL) 

            stcMag.HxL = JY901_Buf[ 1 ];
            stcMag.HxH = JY901_Buf[ 2 ];
            stcMag.HyL = JY901_Buf[ 3 ];
            stcMag.HyH = JY901_Buf[ 4 ];
            stcMag.HzL = JY901_Buf[ 5 ];
            stcMag.HzH = JY901_Buf[ 6 ];

            stcMag.Hx = JY901_Combine( &stcMag.HxH, &stcMag.HxL );
            stcMag.Hy = JY901_Combine( &stcMag.HyH, &stcMag.HyL );
            stcMag.Hz = JY901_Combine( &stcMag.HzH, &stcMag.HzL );   
            
            /*
            stcMag.Hx = ( ( short )stcMag.HxH << 8 ) | stcMag.HxL;
            stcMag.Hy = ( ( short )stcMag.HyH << 8 ) | stcMag.HyL;
            stcMag.Hz = ( ( short )stcMag.HzH << 8 ) | stcMag.HzL;
            */
            break;
            
        case 0x55:  
            // memcpy( &stcDStatus, &JY901_Buf[ 1 ], 8 );
            break;
        case 0x56:  
            memcpy( &stcPress, &JY901_Buf[ 1 ], 8 );
            break;
        case 0x57:  
            memcpy( &stcLonLat, &JY901_Buf[ 1 ], 8 );
            break;
        case 0x58:  
            memcpy( &stcGPSV, &JY901_Buf[ 1 ], 8 );            
            break;

        case 0x59:
            stcQuaternion._0L = JY901_Buf[ 1 ];
            stcQuaternion._0H = JY901_Buf[ 2 ];
            stcQuaternion._1L = JY901_Buf[ 3 ];
            stcQuaternion._1H = JY901_Buf[ 4 ];
            stcQuaternion._2L = JY901_Buf[ 5 ];
            stcQuaternion._2H = JY901_Buf[ 6 ];
            stcQuaternion._3L = JY901_Buf[ 7 ];
            stcQuaternion._3H = JY901_Buf[ 8 ]; 

            stcQuaternion.q0 = JY901_Combine( &stcQuaternion._0H, &stcQuaternion._0L ) / 32768;
            stcQuaternion.q1 = JY901_Combine( &stcQuaternion._1H, &stcQuaternion._1L ) / 32768;
            stcQuaternion.q2 = JY901_Combine( &stcQuaternion._2H, &stcQuaternion._2L ) / 32768;
            stcQuaternion.q3 = JY901_Combine( &stcQuaternion._3H, &stcQuaternion._3L ) / 32768;
            
            break;
            
        case 0x5A:
            
            break;
    }

    #if 0
    sprintf( str,"Time:20%d-%d-%d %d:%d:%.3f\r\n",stcTime.ucYear,stcTime.ucMonth,stcTime.ucDay,stcTime.ucHour,stcTime.ucMinute,(float)stcTime.ucSecond+(float)stcTime.usMiliSecond/1000);
			
	sprintf( str,"Acc:%.3f %.3f %.3f\r\n",(float)stcAcc.a[0]/32768*16,(float)stcAcc.a[1]/32768*16,(float)stcAcc.a[2]/32768*16);

	sprintf( str,"Gyro:%.3f %.3f %.3f\r\n",(float)stcGyro.w[0]/32768*2000,(float)stcGyro.w[1]/32768*2000,(float)stcGyro.w[2]/32768*2000);

	sprintf( str,"Angle:%.3f %.3f %.3f\r\n",(float)stcAngle.Angle[0]/32768*180,(float)stcAngle.Angle[1]/32768*180,(float)stcAngle.Angle[2]/32768*180);

	sprintf( str,"Mag:%d %d %d\r\n",stcMag.h[0],stcMag.h[1],stcMag.h[2]);

	sprintf( str,"Pressure:%ld Height%.2f\r\n",stcPress.lPressure,(float)stcPress.lAltitude/100);

	sprintf( str,"DStatus:%d %d %d %d\r\n",stcDStatus.sDStatus[0],stcDStatus.sDStatus[1],stcDStatus.sDStatus[2],stcDStatus.sDStatus[3]);

	sprintf( str,"Longitude:%ldDeg%.5fm Lattitude:%ldDeg%.5fm\r\n",stcLonLat.lLon/10000000,(double)(stcLonLat.lLon % 10000000)/1e5,stcLonLat.lLat/10000000,(double)(stcLonLat.lLat % 10000000)/1e5);

	sprintf( str,"GPSHeight:%.1fm GPSYaw:%.1fDeg GPSV:%.3fkm/h\r\n\r\n",(float)stcGPSV.sGPSHeight/10,(float)stcGPSV.sGPSYaw/10,(float)stcGPSV.lGPSVelocity/1000);
    #endif
}

