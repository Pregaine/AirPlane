/**************************************************************************/ /**
 * @file	 main.c
 * @version  V0.10
 * $Revision: 0 $
 * $Date: 14/08/01 5:50p $
 * @brief	 Access a SD card formatted in FAT file system
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "NUC505Series.h"
#include "NUC505_Timer.h"
#include "NUC505_I2c.h"
#include "NUC505_Pwm.h"
#include "I2CDev.h"
#include "Typedef.h"
#include "diskio.h"
#include "ff.h"
#include "SD_Write.h"
#include "TimerManager.h"
#include "gpio.h"
#include "MS4525DO.h"
#include "JY901.h"
#include "AirPlane.h"
#include "E32TTL100.h"
#include "Servo.h"

#define G_LED1 PC3_DOUT

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables 																					   */
/*---------------------------------------------------------------------------------------------------------*/
char *ptr;

//---------------------------------------------------------
//--- Initial UART0
//---------------------------------------------------------
void UART0_Init(void)
{
    #if 0
    /* Enable IP clock */
	CLK_EnableModuleClock(UART0_MODULE);

	/* Select IP clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_UART0_SRC_EXT, 0);

    //--- Set GPB0/1 multi-function pins for UART0 RXD and TXD
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB0MFP_Msk)) | SYS_GPB_MFPL_PB0MFP_UART0_TXD;
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk)) | SYS_GPB_MFPL_PB1MFP_UART0_RXD;

    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    
    
	//--- UART0 clock source = XIN
	CLK->CLKDIV3 &= ~(CLK_CLKDIV3_UART0DIV_Msk | CLK_CLKDIV3_UART0SEL_Msk);

	//--- Enable UART0 IP clock
	CLK_EnableModuleClock(UART0_MODULE);

	//--- Reset UART0 IP
	SYS->IPRST1 |= SYS_IPRST1_UART0RST_Msk;
	SYS->IPRST1 &= ~SYS_IPRST1_UART0RST_Msk;

	//--- Set GPB0/1 multi-function pins for UART0 RXD and TXD
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB0MFP_Msk)) | SYS_GPB_MFPL_PB0MFP_UART0_TXD;
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk)) | SYS_GPB_MFPL_PB1MFP_UART0_RXD;

	//--- Configure UART0 attribution and set UART0 Baudrate
	UART0->LINE |= 0x07;				// 8-bit, no parity, 1 stop bit
	UART0->BAUD = 0x30000066;			// 12MHz Crystal in, for 115200
	#endif
}

#if 0
//---------------------------------------------------------
//--- Initial UART1
//---------------------------------------------------------
void UART1_Init(void)
{
	//--- UART1 clock source = XIN
	CLK->CLKDIV3		&= ~(CLK_CLKDIV3_UART1DIV_Msk | CLK_CLKDIV3_UART1SEL_Msk);

	//--- Enable UART1 IP clock
	CLK_EnableModuleClock(UART1_MODULE);

	//--- Reset UART1 IP
	SYS->IPRST1 		|= SYS_IPRST1_UART1RST_Msk;
	SYS->IPRST1 		&= ~SYS_IPRST1_UART1RST_Msk;

	//--- Set GPB6/7 multi-function pins for UART1 RXD and TXD
	SYS->GPB_MFPL		= (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB6MFP_Msk)) | SYS_GPB_MFPL_PB6MFP_UART1_TXD;
	SYS->GPB_MFPL		= (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB7MFP_Msk)) | SYS_GPB_MFPL_PB7MFP_UART1_RXD;

	//--- Configure UART1 attribution and set UART1 Baudrate
	UART1->LINE 		|= 0x07;					// 8-bit, no parity, 1 stop bit
	UART1->BAUD 		= 0x30000066;				// 12MHz Crystal in, for 115200
}


//---------------------------------------------------------
//--- Initial UART2
//---------------------------------------------------------
void UART2_Init(void)
{
	//--- UART2 clock source = XIN
	CLK->CLKDIV3		&= ~(CLK_CLKDIV3_UART2DIV_Msk | CLK_CLKDIV3_UART2SEL_Msk);

	//--- Enable UART2 IP clock
	CLK_EnableModuleClock(UART2_MODULE);

	//--- Reset UART2 IP
	SYS->IPRST1 		|= SYS_IPRST1_UART2RST_Msk;
	SYS->IPRST1 		&= ~SYS_IPRST1_UART2RST_Msk;

	//--- Set GPB10/11 multi-function pins for UART2 RXD and TXD
	SYS->GPB_MFPH		= (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB10MFP_Msk)) | SYS_GPB_MFPH_PB10MFP_UART2_TXD;
	SYS->GPB_MFPH		= (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB11MFP_Msk)) | SYS_GPB_MFPH_PB11MFP_UART2_RXD;

	//--- Configure UART2 and set UART2 Baudrate
	UART2->LINE 		|= 0x07;					// 8-bit, no parity, 1 stop bit
	UART2->BAUD 		= 0x30000066;				// 12MHz Crystal in, for 115200
}
#endif

//---------------------------------------------------------
//--- Initial system clock for NUC505
//---------------------------------------------------------
void SYS_Init(void)
{
    /* Enable  XTAL */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

	//--- Set system clock to PLL and set PLL to 96MHz
	CLK_SetCoreClock(96000000); 				
	
	// don't support clock that > 100MHz
	SystemCoreClockUpdate();

	//--- set APB clock as half of HCLK
	CLK_SetModuleClock( PCLK_MODULE, 0, 1 );

	//--- Initial UART
	//--- MUST also to modify DEBUG_PORT in retarget.c for the UART port that you wanted.
	
	TIM_Init( );
	TM_Init( );
	TM_Start( );
	TIM_StartTickTimer();
	
	//--- Initial SD0 multi-function pin
	SD0_Init();
	SD_Val_Init();

	// PWM_Init( );
	Servo_Init();

	// TIM_Delayms( 100 );
    // IIC_Init( );	

	E32TTL100_Init();
	JY901_Init( );
	I2C_Init( );
	MS4525DO_Init( );
    // ----------------------------------------------
   
}

void Pin_Init()
{
	/* Configure PB.10 as Output mode and PB.11 as Input mode pull up enable then close it */
    // GPIO_SetMode( PB, BIT3, GPIO_MODE_OUTPUT);
	GPIO_SetMode( PC, BIT3, GPIO_MODE_OUTPUT );

	GPIO_SetMode( PC, BIT3, GPIO_MODE_OUTPUT );
	GPIO_SetMode( PC, BIT3, GPIO_MODE_OUTPUT );

	G_LED1 = 1;
}

//-------------------------------------------------------------------------
// This demo program be able to access a SD card formatted in FAT file system.
//
// For NUC505 demo board NHS-NUC505-1-IN-1M11/NHS-NUC505-1-IN-1D11, please
// insert SD card to SD socket A (SD-A on daughter board) before execute program.
//-------------------------------------------------------------------------
int32_t main(void)
{
	// ----- Initial system ---- 
	SYS_Init();

	Pin_Init();

	AirPlane_Init();

	SD_WriteStr( "Airplane on \r\n", strlen( "Airplane on \r\n" ) );

	for( ; ; )
	{
		// SD_Process( );

		MS4525DO_Process( );
        
        JY901_Process( );

        AirPlane_Process( );

        // E32TTL100_Process( );

		TM_Process( );
	}
}

/*** (C) COPYRIGHT 2013 Nuvoton Technology Corp. ***/
