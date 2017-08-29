#ifndef _NUC505_PWM_H_
#define _NUC505_PWM_H_

#define _PWM_CH_OPEN ( ( 0x01 << PWM_CH3 ) | ( 0x01 << PWM_CH2 ) )


extern void PWM_Init( void );
extern void PWM_ChangeDuty( unsigned char ch, unsigned char freq, unsigned char duty );


#endif
