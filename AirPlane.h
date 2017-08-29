#ifndef _AIRPLANE_H_
#define _AIRPLANE_H_

typedef struct
{
    int y_rudder;
    int x_rudder;

    int x_rudder_array[ 5 ];
    int y_rudder_array[ 5 ];

    int moment_right_rudder;    // 當下右舵百分比
	int moment_left_rudder;	    // 當下左舵百分比
	
    TimerID tmr;
    TimerID zforce_tmr;

}_AirPlane;

#define _AIRPLANE_TMR 50    // 30mSec

extern float AirPlane_z_force;

extern void AirPlane_Init( void );
extern void AirPlane_Process( void );

#endif
