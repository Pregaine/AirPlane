#include <stdio.h>
#include "TimerManager.h"
#include "AirPlane.h"
#include "NUC505Series.h"
#include "NUC505_Pwm.h"
#include "JY901.h"
#include "Typedef.h"
#include "Servo.h"
#include "pid.h"

_AirPlane AirPlane;

_PID PID_Yaw = _PID_DEFAULTS;
_PID PID_Row = _PID_DEFAULTS;
_PID PID_Pitch = _PID_DEFAULTS;

void AirPlane_z_force_detect( void );
void AirPlane_PID( void );

float AirPlane_z_force;

void AirPlane_Init( void )
{
    // 單位
    // Up     900uSec    6%  Freq 67Hz 15mSec
    // stand 1470uSec    9%
	// Down  2040uSec 13.6%

	// 單位
    // Up    2.0mSec 10.0%  Freq 50Hz 20mSec
    // stand 1.5mSec  7.5%
	// Down  1.0mSec  5.0%

	AirPlane_z_force = 0.0f;
    
    AirPlane.tmr = TM_CreateTimer( TIMER_ONECYCLE, _AIRPLANE_TMR, NULL );

    AirPlane.zforce_tmr = TM_CreateTimer( TIMER_ONECYCLE, 20, NULL );

    AirPlane.pid_tmr = TM_CreateTimer( TIMER_ONECYCLE, 20, NULL );

    PID_Pitch.Kp = 0;   // 比例係數
    PID_Pitch.Ki = 0;   // 積分係數
    PID_Pitch.Kd = 0;   // 微分係數
    
    PID_Row.Kp = 0;     // 比例係數
    PID_Row.Ki = 0;     // 積分係數
    PID_Row.Kd = 0;     // 微分係數
    
    PID_Yaw.Kp = 0;     // 比例係數
    PID_Yaw.Ki = 0;     // 積分係數
    PID_Yaw.Kd = 0;     // 微分係數    
    // ----------------------------------------------------
}

void AirPlane_Process( void )
{
    // 滚转角（x 轴） Roll=((RollH<<8)|RollL)/32768*180(°) 
    // 俯仰角（y 轴） Pitch=((PitchH<<8)|PitchL)/32768*180(°) 
    // stcAngle.Roll range  = + / - 180
    // stcAngle.Pitch range = + / - 180
	int val = 1;
	static int r_rudder = 0, l_rudder = 0;
	// int x_sum = 0, y_sum = 0;

	AirPlane_PID( );
    
    AirPlane_z_force_detect( );

	if( TM_GetTimerState( AirPlane.tmr ) != TIMER_STOP )
        return;

	TM_StartTimer( AirPlane.tmr );
    //---------------------------------------------
    
    #if( _AIRPLANE_DEBUG == 0 )
    AirPlane.x_rudder = PID_Row.Out;
    AirPlane.y_rudder = PID_Pitch.Out;
    #else
    AirPlane.x_rudder = stcAngle.Roll;
    AirPlane.y_rudder = stcAngle.Pitch;
    #endif
    //---------------------------------------------

    // up +180 -180 down +180 -180 y
    AirPlane.moment_right_rudder = -AirPlane.y_rudder;   // 當下右舵百分比
    AirPlane.moment_left_rudder  = -AirPlane.y_rudder;   // 當下左舵百分比
    // --------------------------------------------

    // right +180 -180 left +180 -180 x
    AirPlane.moment_right_rudder = AirPlane.moment_right_rudder + AirPlane.x_rudder - ( AirPlane_z_force * 20 );
	AirPlane.moment_left_rudder  = AirPlane.moment_left_rudder - AirPlane.x_rudder + ( AirPlane_z_force * 20 );
    // --------------------------------

    // 右舵pwm
    if( r_rudder != AirPlane.moment_right_rudder )
        Servo_Config( 0, ( AirPlane.moment_right_rudder * val ) + 96 );

    // 左舵pwm
    if( l_rudder != AirPlane.moment_left_rudder )
        Servo_Config( 1, ( AirPlane.moment_left_rudder * val ) + 90 );

    r_rudder = AirPlane.moment_right_rudder;
    l_rudder = AirPlane.moment_left_rudder;

    #if 0
    if( flag == 0 )
    {
        index += 90;
    
        if( index == 180 )
            flag = 1;
    }
    else if( flag == 1 )
    {
        index -= 90;

        if( index == 0 )
            flag = 0;    
    }

    // 右舵pwm
    if( r_rudder != index )
        Servo_Config( 0, index );

    // 左舵pwm
    if( l_rudder != index )
        Servo_Config( 1, index );

    r_rudder = index;
    l_rudder = index;
    #endif
}

void AirPlane_z_force_detect( void )
{
    float result_az;

    static float az_array[ 5 ];
    float az_avg = 0;

    u8 i;

    float q0 = stcQuaternion.q0;
    float q1 = stcQuaternion.q1;
    float q2 = stcQuaternion.q2;
    float q3 = stcQuaternion.q3;
    float ax = stcAcc.Ax;
    float ay = stcAcc.Ay;
    float az = stcAcc.Az;
    
    if( TM_GetTimerState( AirPlane.zforce_tmr ) != TIMER_STOP )
        return;

	TM_StartTimer( AirPlane.zforce_tmr );

    result_az = ax*(-2*q0*q2 + 2*q1*q3) + ay*(2*q0*q1 + 2*q2*q3) + az*(-2*q1*q1 - 2*q2*q2 + 1) - 1;

    result_az = -result_az;
    // az_global = ax*(2*q0*q2 + 2*q1*q3) + ay*(-2*q0*q1 + 2*q2*q3) + az*(-2*q1*q1 - 2*q2*q2 + 1) - 1;

    for( i = 0; i <= 3; i ++ )
    {
        az_array[ i ] = az_array[ i + 1 ];
        az_avg += ( az_array[ i ] );
    }

    az_array[ 4 ] = result_az;

    az_avg += ( result_az * 10 );

    AirPlane_z_force = az_avg / 5;
}


void AirPlane_PID()
{
    if( TM_GetTimerState( AirPlane.tmr ) != TIMER_STOP )
        return;

	TM_StartTimer( AirPlane.tmr );

	PID_Pitch.Err = PID_Pitch.Ref - stcAngle.Pitch;

	PID_Row.Err = PID_Row.Ref - stcAngle.Roll;

	PID_Yaw.Err = PID_Yaw.Ref - stcAngle.Yaw;

    PID_Pitch.calc( &PID_Pitch );

	PID_Row.calc( &PID_Row );

	PID_Yaw.calc( &PID_Yaw );
}
