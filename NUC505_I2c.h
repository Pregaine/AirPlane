#ifndef _NUC505_I2C_H_
#define _NUC505_I2C_H_

typedef enum
{
    _IIC_Start = 0x08,
    _IIC_Master_Repeat_Start            = 0x10,
    _IIC_Master_Transmit_Address_ACK    = 0x18,
    _IIC_Master_Transmit_Address_NACK   = 0x20,
    _IIC_Master_Transmit_Data_ACK       = 0x28,
    _IIC_Master_Transmit_Data_NACK      = 0x30,
    _IIC_Master_Arbitration_Lost        = 0x38,
    _IIC_Master_Receive_Address_ACK     = 0x40,
    _IIC_Master_Receive_Address_NACK    = 0x48,
    _IIC_Master_Receive_Data_ACK        = 0x50,
    _IIC_Master_Receive_Data_NACK       = 0x58,
    _IIC_Bus_Error = 0x00

}_IIC_STATUS;


extern void IIC_Init( void );
extern unsigned int IIC_SendStart( I2C_T *port );
extern unsigned int IIC_SendStop( I2C_T *port );

#endif

