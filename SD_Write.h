#ifndef _SD_WRITE_H_
#define _SD_WRITE_H_

#define _SD_DebugMsePrint 0

extern void SD0_Init(void);
extern void SD_Val_Init(void);
extern void SD_Process(void );
extern unsigned char SD_WriteStr( char * str, unsigned char len);

#endif
