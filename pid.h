#ifndef _PID_H_
#define _PID_H_

// 定義PID計算用到的結構體類型
typedef struct 
{

float   Ref;                                        //輸入：系統待調節量的給定值

// PID控制器部分
float   Kp;                                         //參數：比例繫數
float   Ki;                                         //參數：積分繫數
float   Kd;                                         //參數：微分繫數

float   T;                                          //參數：離散化系統的採樣周期

float   a0;                                         //變數：a0
float   a1;                                         //變數: a1
float   a2;                                         //變數: a2

float   Err;                                        //變數：當前的偏差e(k)
float   Err_1;                                      //歷史：前一步的偏差e(k-1)
float   Err_2;                                      //歷史：前前一步的偏差e(k-2)

float   Out;                                        //輸出：PID控制器的輸出u(k)
float   Out_1;                                      //歷史：PID控制器前一步的輸出u(k-1)
float   OutMax;                                     //參數：PID控制器的最大輸出
float   OutMin;                                     //參數：PID控制器的最小輸出

void( *calc )( );                                   //函數指針：指向PID計算函數

}_PID;

// 定義PID控制器的初始值
#define _PID_DEFAULTS { 0, 0, 0, 0, 0.02f, 0, 0, 0, 0, 0, 0, 0, 0, 45, -45, pid_calc } // 加與不加強制類型轉換都沒影響

#define _PID_DEBUG 1

// 函數聲明
void pid_calc( _PID *p );

#endif
