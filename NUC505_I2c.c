#include <stdio.h>
#include "NUC505Series.h"
#include "NUC505_I2c.h"
#include "Typedef.h"

void IIC_Init( void )
{

    CLK_EnableModuleClock(I2C0_MODULE);

    /* Set GPA14,GPA15 multi-function pins for I2C0 */
    SYS->GPA_MFPH = (SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA14MFP_Msk) ) | SYS_GPA_MFPH_PA14MFP_I2C0_SCL;
    SYS->GPA_MFPH = (SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA15MFP_Msk) ) | SYS_GPA_MFPH_PA15MFP_I2C0_SDA;

    
    /* Open I2C0 and set clock to 100k */
    I2C_Open( I2C0, 100000 );

    /* Get I2C0 Bus Clock */
    printf( "I2C clock %d Hz\n", I2C_GetBusClockFreq(I2C0) );
}

/*******************************************************************************

*******************************************************************************/
U16 IIC_SendStart( I2C_T *port )
{
    U32 timeout = 0xFFFFF;
    	
	/* I2C as master sends START signal */
    I2C_SET_CONTROL_REG( port, I2C_STA );

    while( timeout > 0 )
    {
        if( I2C_GET_STATUS( port ) == _IIC_Start )
            break;
        
        timeout--;
    }
        
    return timeout ? _SUCCESS : _FAIL;
}

/*******************************************************************************

*******************************************************************************/
U16 IIC_SendStop( I2C_T *port )
{
    U32 timeout = 0xFFFFF;

    I2C_SET_CONTROL_REG( port, I2C_STO | I2C_SI );
    
    while ( timeout > 0 )
    {
        timeout--;

        if( I2C_GET_STATUS( port ) != _IIC_Bus_Error )
            break;
    }
    
    return timeout ? _SUCCESS : _FAIL;
}

#if 0
/*******************************************************************************

*******************************************************************************/
U16 IIC_SendRepeatStart(volatile I2C_Reg *port)
{
    U16 timeout = 0xFFFF;

    if (!port->STAT.S)
	{
#ifdef IIC_DEBUG
    	DBG_Print("STAT.S=0 ");
#endif
		return FAIL;
	}

    port->CON.RSEN = 1;
    while (port->CON.SEN && timeout)
        timeout--;

#ifdef IIC_DEBUG
	DBG_Print(timeout ? "RS " : "RS->Timeout ");
#endif

    return timeout ? SUCCESS : FAIL;
}
#endif


