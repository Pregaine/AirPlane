#ifndef _NUC505_TIMER_H_
#define _NUC505_TIMER_H_

#define TIM_GetTimeDiff(firstT, lastT) ((firstT > lastT) ? (0xFFFFFFFFL - firstT + 1) + lastT : lastT - firstT)
typedef void (*TIM_CallBackFun)( unsigned char timer);

extern uint32_t system_Tick;
extern void TIM_Init( void );
extern void TIM_GetTickCount( unsigned long int *value);
extern void TIM_TurnOn( unsigned char timer);
extern void TIM_TurnOff( unsigned char timer);
extern void TIM_Delayms( unsigned long int t);
extern void TIM_SetTimer( unsigned char timer, unsigned long int period, TIM_CallBackFun fun);
extern void TIM_IntDisable(unsigned char timer);
extern void TIM_IntEnable(unsigned char timer);
extern void TIM_StartTickTimer( void );
extern void TIM_StopTickTimer( void );
extern unsigned char TIM_IsOn(unsigned char timer);
extern void TIM_Passing2Function_Pointer( TIM_CallBackFun fun );

#endif 
