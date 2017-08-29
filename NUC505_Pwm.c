#include <stdio.h>
#include "NUC505Series.h"
#include "NUC505_Pwm.h"
#include "Typedef.h"

void PWM_Init()
{
    CLK_EnableModuleClock(PWM_MODULE);

    /* Set EXT as PWM channel 0~3 clock source */
    CLK_SetModuleClock( PWM_MODULE, CLK_PWM_SRC_EXT, 0 );

    /* Set GPB10~GPB13 multi-function pins for PWM Channel 0~3 */
    // SYS->GPB_MFPH  = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB10MFP_Msk) ) | SYS_GPB_MFPH_PB10MFP_PWM_CH0;
    // SYS->GPB_MFPH  = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB11MFP_Msk) ) | SYS_GPB_MFPH_PB11MFP_PWM_CH1;
    // SYS->GPB_MFPH  = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB12MFP_Msk) ) | SYS_GPB_MFPH_PB12MFP_PWM_CH2;
    // SYS->GPB_MFPH  = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB13MFP_Msk) ) | SYS_GPB_MFPH_PB13MFP_PWM_CH3;
    
    SYS->GPB_MFPH  = (SYS->GPB_MFPH & ( ~SYS_GPB_MFPH_PB10MFP_Msk ) ) | SYS_GPB_MFPH_PB10MFP_PWM_CH0;
    SYS->GPC_MFPH  = (SYS->GPC_MFPH & ( ~SYS_GPC_MFPH_PC12MFP_Msk ) ) | SYS_GPC_MFPH_PC12MFP_PWM_CH3;
    

    // PWM2 frequency is 100Hz, duty 30%,
    PWM_ConfigOutputChannel( PWM, PWM_CH0, 50, 5 );
    // PWM_EnableDeadZone( PWM, PWM_CH0, 2 );

    // PWM3 frequency is 300Hz, duty 50%
    PWM_ConfigOutputChannel( PWM, PWM_CH3, 50, 5 );
    // PWM_EnableDeadZone( PWM, PWM_CH3, 2 );

    // Enable output of all PWM channels
    PWM_EnableOutput( PWM, 0xF );
    
    // Enable PWM channel 0 period interrupt, use channel 0 to measure time.
    // PWM_EnablePeriodInt(PWM, 0, 0);
    // NVIC_EnableIRQ(PWM0_IRQn);

    // Start
    PWM_Start( PWM, 0xF );

}

void PWM_ChangeDuty( unsigned char ch, unsigned char freq, unsigned char duty )
{
    static U8 bk_ch2_duty = 0, bk_ch3_duty = 0;

    if( ch == PWM_CH2 )
    {
        if( duty == bk_ch2_duty )
            return;
    
        bk_ch2_duty = duty;  
    }
    else if( ch == PWM_CH3 )
    {
        if( duty == bk_ch3_duty )
            return;

        bk_ch3_duty = duty;
    }
    else
        return;    

    PWM_Stop( PWM, 0x01 << ch );

    PWM_ConfigOutputChannel( PWM, ch, freq, duty );

    PWM_Start( PWM, 0x01 << ch );
}

