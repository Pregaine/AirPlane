#ifndef _TIMER_MANAGER_H_
#define _TIMER_MANAGER_H_

typedef unsigned int TimerID;
typedef void (*TM_OnTimerEvent)( TimerID id );

#define TIMER_STOP		    2
#define TIMER_RUNNING       1
#define TIMER_INVALID	    0

#define TIMER_USED_MASK	    0x80
#define TIMER_CONTINUOUS    0x01
#define TIMER_ONECYCLE      0x00

extern void TM_Init( void );

extern void TM_Start( void );

extern void TM_Stop( void );

extern TimerID TM_CreateTimer( unsigned char mode, unsigned long priod, TM_OnTimerEvent event);

extern TimerID TM_CreateIntTimer( unsigned char mode, unsigned long priod, TM_OnTimerEvent event);

extern void TM_Process( void );

extern void TM_FreeTimer(TimerID id);

extern void TM_StartTimer(TimerID id);

extern void TM_StopTimer(TimerID id);

extern unsigned char TM_GetTimerState(TimerID id);

extern void TM_ConfigTimer(TimerID id, unsigned long priod, TM_OnTimerEvent event);

extern void TM_Process( void );

extern void TM_PrintResource( void );

#endif

