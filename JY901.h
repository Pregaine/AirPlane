#ifndef _JY901_H_
#define _JY901_H_

#define SAVE 			0x00
#define CALSW 		0x01
#define RSW 			0x02
#define RRATE			0x03
#define BAUD 			0x04
#define AXOFFSET	0x05
#define AYOFFSET	0x06
#define AZOFFSET	0x07
#define GXOFFSET	0x08
#define GYOFFSET	0x09
#define GZOFFSET	0x0a
#define HXOFFSET	0x0b
#define HYOFFSET	0x0c
#define HZOFFSET	0x0d
#define D0MODE		0x0e
#define D1MODE		0x0f
#define D2MODE		0x10
#define D3MODE		0x11
#define D0PWMH		0x12
#define D1PWMH		0x13
#define D2PWMH		0x14
#define D3PWMH		0x15
#define D0PWMT		0x16
#define D1PWMT		0x17
#define D2PWMT		0x18
#define D3PWMT		0x19
#define IICADDR		0x1a
#define LEDOFF 		0x1b
#define GPSBAUD		0x1c

#define YYMM				0x30
#define DDHH				0x31
#define MMSS				0x32
#define MS					0x33
#define AX					0x34
#define AY					0x35
#define AZ					0x36
#define GX					0x37
#define GY					0x38
#define GZ					0x39
#define HX					0x3a
#define HY					0x3b
#define HZ					0x3c			
#define _Roll				0x3d
#define _Pitch			0x3e
#define _Yaw				0x3f
#define TEMP				0x40
#define D0Status		0x41
#define D1Status		0x42
#define D2Status		0x43
#define D3Status		0x44
#define PressureL		0x45
#define PressureH		0x46
#define HeightL			0x47
#define HeightH			0x48
#define LonL				0x49
#define LonH				0x4a
#define LatL				0x4b
#define LatH				0x4c
#define GPSHeight   0x4d
#define GPSYAW      0x4e
#define GPSVL				0x4f
#define GPSVH				0x50
      
#define DIO_MODE_AIN 0
#define DIO_MODE_DIN 1
#define DIO_MODE_DOH 2
#define DIO_MODE_DOL 3
#define DIO_MODE_DOPWM 4
#define DIO_MODE_GPS 5		

/*
RATE：回传速率 
  0x01：0.1Hz 
  0x02：0.5Hz 
  0x03：1Hz 
  0x04：2Hz 
  0x05：5Hz 
  0x06：10Hz（默认） 
  0x07：20Hz 
  0x08：50Hz 
  0x09：100Hz 
  0x0a：200Hz 
  0x0b：单次 
  0x0c：不输出 
*/
#define _RATE_1HZ   0x03
#define _RATE_10HZ  0x06
#define _RATE_100HZ 0x09
#define _RATE_200HZ 0x0A

typedef struct
{
	unsigned char ucYear;
	unsigned char ucMonth;
	unsigned char ucDay;
	unsigned char ucHour;
	unsigned char ucMinute;
	unsigned char ucSecond;
	unsigned short usMiliSecond;
	
}_STime;

typedef struct
{
	unsigned char AxL;
	unsigned char AxH;

	unsigned char AyL;
	unsigned char AyH;

	unsigned char AzL;
	unsigned char AzH;

	unsigned char TL;
	unsigned char TH;

	float Ax;
	float Ay;
	float Az;
	
}_SAcc;

typedef struct
{
    unsigned char WxL;
    unsigned char WxH;

    unsigned char WyL;
    unsigned char WyH;

    unsigned char WzL;
    unsigned char WzH;

    unsigned char TL;
    unsigned char TH;
    
    float Wx;
	float Wy;
	float Wz;
	
}_SGyro;

typedef struct
{
	unsigned char RollL;
	unsigned char RollH;

	unsigned char PitchL;
	unsigned char PitchH;

	unsigned char YawL;
	unsigned char YawH;

	unsigned char TL;
	unsigned char TH;

	float Roll;
	float Pitch;
	float Yaw;
	
}_SAngle;

typedef struct
{
	unsigned char HxL;
	unsigned char HxH;
	unsigned char HyL;
	unsigned char HyH;
	unsigned char HzL;
	unsigned char HzH;

	short Hx;
	short Hy;
	short Hz;

}_SMag;

typedef struct
{
	short sDStatus[4];
}_SDStatus;

typedef struct
{
	long lPressure;
	long lAltitude;
	
}_SPress;

typedef struct
{
	long lLon;
	long lLat;

	long lontitude;
	long latitude;
	
}_SLonLat;

typedef struct
{
	short sGPSHeight;
	short sGPSYaw;
	long lGPSVelocity;
	
}_SGPSV;

typedef struct
{
    unsigned char _0L;
    unsigned char _0H;
    unsigned char _1L;
    unsigned char _1H;
    unsigned char _2L;
    unsigned char _2H;
    unsigned char _3L;
    unsigned char _3H;

    float q0;
    float q1;
    float q2;
    float q3;
    
}_SQuaternion;

extern _STime stcTime;
extern _SAcc stcAcc;
extern _SGyro stcGyro;
extern _SAngle stcAngle;
extern _SMag stcMag;
extern _SDStatus stcDStatus;
extern _SPress stcPress;
extern _SLonLat stcLonLat;
extern _SGPSV stcGPSV;
extern _SQuaternion stcQuaternion;
extern volatile uint32_t g_u32comRbytes;


extern void JY901_Init( void );
extern void JY901_Process( void );
extern void JY901_DebugMsg( unsigned int timer );

#endif
