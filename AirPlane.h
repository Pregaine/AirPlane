#ifndef _AIRPLANE_H_
#define _AIRPLANE_H_

#include "pid.h"

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
    TimerID pid_tmr;

}_AirPlane;

#define _AIRPLANE_TMR 50    // 30mSec
#define _AIRPLANE_DEBUG 1

extern float AirPlane_z_force;
extern _PID PID_Pitch;
extern _PID PID_Row;
extern _PID PID_Yaw;
extern void AirPlane_Init( void );
extern void AirPlane_Process( void );

#endif
