/*****************************************************************************
 *          Copyright c E-LEAD Electronics All Rights Reserved
 *****************************************************************************			
 * Description:
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "Typedef.h"
#include "NUC505Series.h"
#include "NUC505_Timer.h"
#include "Servo.h"
#include "gpio.h"

#define G_LED1 PC3_DOUT


static TIM_CallBackFun _callbackFun[4];
volatile static U32 _tickCount;

uint32_t system_Tick = 0;

//---------------------------------------------------------
//--- ISR for Timer0 interrupt
//---------------------------------------------------------

// System Tick Timer
void TMR2_IRQHandler(void)
{
    Servo_Det();    

    // G_LED1 = ~G_LED1;

	// clear timer interrupt flag
	TIMER_ClearIntFlag( TIMER2 );

	/* To avoid the synchronization issue between system and APB clock domain */
    TIMER_GetIntFlag( TIMER2 );
}

// TMR1
void TMR1_IRQHandler(void)
{
    system_Tick++;
	_tickCount++;

	if (_callbackFun[0] != NULL)
		_callbackFun[0](0);
	
	// clear timer interrupt flag
	TIMER_ClearIntFlag( TIMER1 );

	/* To avoid the synchronization issue between system and APB clock domain */
    TIMER_GetIntFlag(TIMER1);
}


void TIM_Init()
{
	U8 i;
	_tickCount = 0;
	
	for ( i = 0; i < ArrayLength(_callbackFun); ++i )
		_callbackFun[i] = 0;

}

void TIM_StopTickTimer()
{
	TIMER_Stop( TIMER0 );
}

// System Tick Timer
void TIM_StartTickTimer()
{
	// ----- Initial Timer2 for system tick.
    // ----- The ISR for Timer2 is TMR2_IRQHandler() 
    // that defined at startup_NUC505Series.s
    CLK_EnableModuleClock( TMR2_MODULE );                     // Enable IP clock
    CLK_SetModuleClock( TMR2_MODULE, CLK_TMR2_SRC_EXT, 0 );   // Select IP clock source
    TIMER_Open( TIMER2, TIMER_PERIODIC_MODE, 20000 );         // Set timer frequency to 20000HZ. 1 tick = 50uSec
    TIMER_EnableInt( TIMER2 );                        		  // Enable timer interrupt
    NVIC_EnableIRQ( TMR2_IRQn );                      		  // Enable External Interrupt in NVIC for Timer0
	TIMER_Start( TIMER2 );    
}

void TIM_GetTickCount( U32 *value )
{

	TIMER_Stop( TIMER0 );
	
	*value = _tickCount;
	
	TIMER_Start( TIMER0 );
}

U8 TIM_IsOn( U8 timer )
{
	switch (timer)
	{
		case 0:
			return TIMER_IS_ACTIVE( TIMER0 );
		case 1:
			return TIMER_IS_ACTIVE( TIMER1 );
		case 2:
			return TIMER_IS_ACTIVE( TIMER2 );
	}

	return 0;
}

void TIM_TurnOn( U8 timer )
{
	switch ( timer )
	{
		case 0:
			break;
	
		case 1:
			TIMER_Start( TIMER1 );
			break;
		
		case 2:			
			break;
	}
}

void TIM_TurnOff(U8 timer)
{
	switch (timer)
	{
		case 0: 
			break;

		case 1:
			TIMER_Stop( TIMER1 );
			break;
			
		case 2:
			break;
	}
}


/*unit of period : 1us*/
void TIM_SetTimer( U8 timer, U32 period, TIM_CallBackFun fun )
{
	switch (timer)
	{
		// case 0:			
		//	break;

		case 1:	
			// Timer 1
			// ----- Initial Timer1 for system tick.
    		// ----- The ISR for Timer1 is TMR1_IRQHandler() that defined at startup_NUC505Series.s
    		CLK_EnableModuleClock( TMR1_MODULE );                     	// Enable IP clock
    		CLK_SetModuleClock( TMR1_MODULE, CLK_TMR1_SRC_EXT, 0 );   	// Select IP clock source
    		TIMER_Open( TIMER1, TIMER_PERIODIC_MODE, period );   		// Set timer frequency to period HZ.
    		TIMER_EnableInt( TIMER1 );                        			// Enable timer interrupt
    		NVIC_EnableIRQ( TMR1_IRQn );                      			// Enable External Interrupt in NVIC for Timer0

			_callbackFun[ 0 ] = fun;
			
			break;

		case 2:

			_callbackFun[ 1 ] = fun;
			
			break;
	}
}


void TIM_Delayms( U32 t )
{
	TIMER_Delay( TIMER2, t * 1000 );
}

void TIM_IntDisable( U8 timer )
{
	switch ( timer )
	{
		
		case 1:
			
			TIMER_DisableInt( TIMER1 );

			break;
		
		#if 0
		case 0:
		case 2:
			break;
		#endif
	}
}


void TIM_IntEnable( U8 timer )
{
	switch (timer)
	{
		case 0:
			
			break;

		case 1:

			TIMER_EnableInt( TIMER1 );

			break;
		
		case 2:

			break;
	}
}


