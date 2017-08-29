#include <stdio.h>
#include "NUC505Series.h"
#include "Typedef.h"
#include "gpio.h"
#include "Servo.h"

typedef struct
{
    u16 hi_cnt;
    u16 lo_cnt;

    u16 bk_hi_cnt;
    u16 bk_lo_cnt;
    
    u16 val;
    unsigned char flag;
    
}_Servo;

#define G_LED1 PC3_DOUT

u8 ch_offset[] = { 6, 0 };


// 2000uSec ~ 1000uSec 180 ~ 0
#define _SERVO_STEP  30
#define _SERVO_TMR  400     // 400 * 50 uSec = 20mSec
#define _SERVO_CHMAX  2

#define SERVO_CH2_PORT PB10_DOUT
#define SERVO_CH3_PORT PC12_DOUT

// ch 1 PB10
// ch 2 PC12
_Servo Servo_Ch[ 2 ];

// SYS->GPB_MFPH  = (SYS->GPB_MFPH & ( ~SYS_GPB_MFPH_PB10MFP_Msk ) ) | SYS_GPB_MFPH_PB10MFP_PWM_CH0;
// SYS->GPC_MFPH  = (SYS->GPC_MFPH & ( ~SYS_GPC_MFPH_PC12MFP_Msk ) ) | SYS_GPC_MFPH_PC12MFP_PWM_CH3;

void Servo_Config( u8 ch, s16 degree )
{
    u8 offset = ch_offset[ ch ];

    if( degree > ( 180 - 45 + offset ) )
        degree = ( 180 - 45 + offset );

    if( degree < ( 45 + offset ) )
        degree = ( 45 + offset );

    // 20 * 50uSec -> 1mSec 
    Servo_Ch[ ch ].hi_cnt = ( ( degree * _SERVO_STEP ) / 180 ) + 19;

    Servo_Ch[ ch ].lo_cnt = _SERVO_TMR - Servo_Ch[ ch ].hi_cnt;
}


void Servo_Init()
{   
    // 50uSec = 20000uSec / 400
    // 20mSec = 20000uSec = 400cnt
    u8 i;

    GPIO_SetMode( PB, BIT10, GPIO_MODE_OUTPUT );
    GPIO_SetMode( PC, BIT12, GPIO_MODE_OUTPUT );

    SERVO_CH2_PORT = 0;
    SERVO_CH3_PORT = 0;

    for( i = 0; i < _SERVO_CHMAX; i ++ )
    {
        Servo_Ch[ i ].flag = 0;
        Servo_Ch[ i ].val = 0;
        Servo_Ch[ i ].bk_hi_cnt = 0;
        Servo_Ch[ i ].bk_lo_cnt = 0;
    }

    Servo_Config( 0, 90 );
    Servo_Config( 1, 90 );
}

void Servo_Det( void )
{
    u8 i;

    // PB10_DOUT = ~PB10_DOUT;
    // PC12_DOUT = ~PC12_DOUT;
   
    for( i = 0; i < _SERVO_CHMAX; i++ )
    {
        if( Servo_Ch[ i ].val > 0 )
        {
            Servo_Ch[ i ].val --;
        }
        else
        {
            if( Servo_Ch[ i ].flag == 0 )
            {
                Servo_Ch[ i ].val  = Servo_Ch[ i ].bk_hi_cnt;
                Servo_Ch[ i ].flag = 1;

                if( i == 0 )
                {
                    SERVO_CH2_PORT = 1;    
                }
                else if( i == 1 )
                {
                    SERVO_CH3_PORT = 1;
                }
            }
            else if( Servo_Ch[ i ].flag == 1 )
            {
                Servo_Ch[ i ].bk_lo_cnt = Servo_Ch[ i ].lo_cnt;
                Servo_Ch[ i ].bk_hi_cnt = Servo_Ch[ i ].hi_cnt;
                // ---------------------------------------
            
                Servo_Ch[ i ].val  = Servo_Ch[ i ].bk_lo_cnt;
                Servo_Ch[ i ].flag = 0;
                // ----------------------------------------
                
                if( i == 0 )
                {
                    SERVO_CH2_PORT = 0;
                }
                else if( i == 1 )
                {
                    SERVO_CH3_PORT = 0;
                }
            }
        }
    }
}
