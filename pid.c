#include "pid.h"
//===================函數定義========================
/****************************************************
*說    明：
*    （1）PID控制器預設為PI調節器
*    （2）使用了條件編譯進行功能切換：節省計算時間
*         在校正PID參數時，使用巨集定義將PID_DEBUG設為1；
*         當參數校正完成後，使用巨集定義將PID_DEBUG設為0，同時，在初始化時
*         直接為p->a0、p->a1、p->a2賦值
****************************************************/
void pid_calc( _PID *p )
{
    //使用條件編譯進行功能切換
    #if ( _PID_DEBUG )

    float a0,a1,a2;
    
    //計算中間變數a0、a1、a2
    a0 = p->Kp + ( p->Ki * p->T ) + ( p->Kd / p->T );
    a1 = p->Kp + ( 2 * p->Kd / p->T );
    a2 = ( p->Kd / p->T );

    //計算PID控制器的輸出
    p->Out = p->Out_1 + ( a0 * p->Err ) - ( a1 * p->Err_1 ) + ( a2 * p->Err_2 );

    #else

    //計算PID控制器的輸出
    p->Out = p->Out_1 + ( p->a0 * p->Err ) - ( p->a1 * p->Err_1 ) + ( p->a2 * p->Err_2 );

    #endif
    
    // 輸出限幅
    if( p->Out > p->OutMax )
        p->Out = p->OutMax;
        
    if( p->Out < p->OutMin )
        p->Out = p->OutMin;
    
    //為下步計算做準備
    p->Out_1 = p->Out;
    p->Err_2 = p->Err_1;
    p->Err_1 = p->Err;
}
