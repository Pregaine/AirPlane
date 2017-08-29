#ifndef _MS4525DO_H_
#define _MS4525DO_H_

#define _MS4525DO_Ok    0x00
#define _MS4525DO_Busy  0x01
#define _MS4525DO_Slate 0x02
#define _MS4525DO_Error 

// InterFace Type I 0x28
// InterFace Type J 0x36

#define _MS4525DO_Addr 0x28

typedef struct
{
    unsigned char status;

    unsigned short rawpressure;

    unsigned short rawtemperature;

    float airspeed;

    float temperature;

    TimerID tmr;

}_MS4525DO;

#define _MS4525DO_TMR 1000

extern void MS4525DO_Init( void );
extern void MS4525DO_Process( void );

extern _MS4525DO MS4525DO;

#endif
