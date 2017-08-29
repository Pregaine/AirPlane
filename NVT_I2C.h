/*============================================================================*
 * O     O          __                   ______  __                           *
 *  \   /      /\  / /_      _    __    / /___/ / /_     _                    *
 *   [+]      /  \/ / \\    //__ / /__ / /____ / / \\   //                    *
 *  /   \    / /\  /   \\__// --/ /---/ /----// /   \\_//                     *
 * O     O  /_/  \/     \__/    \_\/ /_/     /_/ ____/_/                      *
 *                                                                            *
 *                                                                            *
 * Multi-Rotor controller firmware for Nuvoton Cortex M4 series               *
 *                                                                            *
 * Written by by T.L. Shen for Nuvoton Technology.                            *
 * tlshen@nuvoton.com/tzulan611126@gmail.com                                  *
 *                                                                            *
 *============================================================================*
 */
#ifndef _DRVI2C_NVT_H
#define _DRVI2C_NVT_H
#include <stdint.h>
#define MAX_I2C_DATA_LEN 200

extern void NVT_I2C_Init(void);
extern void  NVT_SetDeviceAddress(uint8_t devAddr);

extern uint8_t NVT_WriteByteContinue_addr8(uint8_t address,uint8_t* data, uint8_t len);
extern uint8_t NVT_ReadByteContinue_addr8(uint8_t address,uint8_t* data, uint8_t len, uint16_t timeout);

#endif
