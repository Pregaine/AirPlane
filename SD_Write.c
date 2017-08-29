
#include <stdio.h>
#include <string.h>
#include "TimerManager.h"
#include "NUC505Series.h"
#include "NUC505_Timer.h"
#include "diskio.h"
#include "ff.h"
#include "SD_Write.h"

void SD_Process(void);
void SD_get_line(char * buff, int len);
BYTE SD_WaitCmd(char * ptr);

BYTE SD_FiCmd(void);
BYTE SD_FlCmd(void);
BYTE SD_FoCmd(void);
BYTE SD_FeCmd(void);
BYTE SD_FwCmd(void);
BYTE SD_FcCmd(void);
BYTE SD_Idle(void);

// type-definition: 'pt2Function' now can be used as type
typedef BYTE(*pt2Function) (void);

pt2Function FuncArray[] =
{
	SD_FlCmd, SD_FoCmd, SD_FeCmd, SD_FwCmd, SD_FcCmd, SD_Idle
};

char *SD_DebugStr[] = 
{
	"SD_Flcmd\r\n",
	"SD_Focmd\r\n",
	"SD_Fecmd\r\n",
	"SD_Fwcmd\r\n",
	"SD_FcCmd\r\n"
	"SD_Idle\r\n",
};


typedef enum 
{
_fl = 0, 
_fo, 
_fe, 
_fw, 
_fc, 
_idle
//_fi 
} _sd_state;


/*---------------------------------------------------------------------------------------------------------*/
/* Global variables 																					   */
/*---------------------------------------------------------------------------------------------------------*/
static UINT 	blen = 16 * 1024;
volatile UINT	Timer = 0, Timer2 = 0; /* Performance timer */
DWORD			acc_size; /* Work register for fs command */
WORD			acc_files, acc_dirs;
FILINFO 		Finfo;
FATFS			FatFs[_VOLUMES]; /* File system object for logical drive */
char			SD_Line[64]; /* Console input buffer */

#if _USE_LFN
char			Lfname[512];

#endif

#define _SD_BUFFERSIZE 1024

BYTE			SD_Input_First = 0;
BYTE			SD_Output_First = 0;
BYTE			SD_Buffer_Size = 0;

BYTE			Buff[_SD_BUFFERSIZE]; /* Working buffer */
BYTE			SD_Drv = 0; // select SD0

char *			ptr2;
long			p1, p2, p3;
BYTE *			buf;
FATFS * 		fs; /* Pointer to file system object */
FRESULT 		res;

DIR 			dir; /* Directory object */
FIL 			file1, file2, SD_Tar; /* File objects */
UINT			s1, s2, cnt, buf_size;

BYTE    SD_state;
BYTE    SD_WriteTimeOut;

static TimerID SD_TimeOutTmr;

/*
static const BYTE ft[] =
{
	0, 12, 16, 32
};
*/

DWORD ofs = 0, sect = 0;

void Delay(uint32_t delayCnt)
{
	while (delayCnt--)
	{
		__NOP();
		__NOP();
	}
}


/** Number on countu**/
int n_tu(int number, int count)
{
	int 			result = 1;

	while (count-- > 0)
	{
		result				*= number;
	}

	return result;
}


//***Convert float to string ***//
void float_to_string(float f, char * r)
{
	long long int	length, length2, i, number, position, sign;
	float			number2;

	sign				= -1;						// -1 == positive number

	if (f < 0)
	{
		sign				= '-';
		f					*= -1;
	}

	number2 			= f;
	number				= f;
	length				= 0;						// size of decimal part
	length2 			= 0;						// size of tenth

	/* calculate length2 tenth part */
	while ((number2 - (float) number) != 0.0f && ! ((number2 - (float) number) < 0.0f))
	{
		number2 			= f * (n_tu(10.0, length2 + 1));
		number				= number2;
		length2++;
	}

	/* calculate length decimal part*/
	for (length = (f > 1) ? 0: 1; f > 1; length++)
	{
		f					/= 10;
	}

	position			= length;
	length				= length + 1 + length2;
	number				= number2;

	if (sign == '-')
	{
		length++;
		position++;
	}

	for (i = length; i >= 0; i--)
	{
		if (i == (length))
		{
			r[i]				= 0;
		}
		else if (i == (position))
		{
			r[i]				= '.';
		}
		else if (sign == '-' && i == 0)
		{
			r[i]				= '-';
		}
		else 
		{
			r[i]				= (number % 10) + '0';
			number				/= 10;
		}
	}
}


/*--------------------------------------------------------------------------*/
/* Monitor																	*/

/*----------------------------------------------*/
/* Get a value of the string					*/
/*----------------------------------------------*/
/*	"123 -5   0x3ff 0b1111 0377  w "
		^							1st call returns 123 and next ptr
		   ^						2nd call returns -5 and next ptr
				   ^				3rd call returns 1023 and next ptr
						  ^ 		4th call returns 15 and next ptr
							   ^	5th call returns 255 and next ptr
								  ^ 6th call fails and returns 0
*/
int xatoi(/* 0:Failed, 1:Successful */
TCHAR * *		str, /* Pointer to pointer to the string */
long * res /* Pointer to a variable to store the value */
)
{
	unsigned long	val;
	unsigned char	r, s = 0;
	TCHAR			c;


	*res				= 0;

	while ((c = * *str) == ' ')
	{
		(*str) ++;									/* Skip leading spaces */
	}

	if (c == '-')
	{ /* negative? */
		s					= 1;
		c					= * (++ (*str));
	}

	if (c == '0')
	{
		c					= * (++ (*str));

		switch (c)
		{
			case 'x': /* hexadecimal */
				r = 16;
				c = * (++ (*str));
				break;

			case 'b': /* binary */
				r = 2;
				c = * (++ (*str));
				break;

			default:
				if (c <= ' ')
				{
					return 1; /* single zero */
				}

				if (c < '0' || c > '9')
				{
					return 0; /* invalid char */
				}

				r = 8; /* octal */
		}
	}
	else 
	{
		if (c < '0' || c > '9')
		{
			return 0; /* EOL or invalid char */
		}

		r					= 10;					/* decimal */
	}

	val 				= 0;

	while (c > ' ')
	{
		if (c >= 'a')
		{
			c					-= 0x20;
		}

		c					-= '0';

		if (c >= 17)
		{
			c					-= 7;

			if (c <= 9)
			{
				return 0; /* invalid char */
			}
		}

		if (c >= r)
		{
			return 0; /* invalid char for current radix */
		}

		val 				= val * r + c;
		c					= * (++ (*str));
	}

	if (s)
	{
		val 				= 0 - val;				/* apply sign if needed */
	}

	*res				= val;
	return 1;
}


/*----------------------------------------------*/
/* Dump a block of byte array					*/
/*----------------------------------------------*/
void put_dump(const unsigned char * buff, /* Pointer to the byte array to be dumped */
unsigned long	addr, /* Heading address value */
int cnt /* Number of bytes to be dumped */
)
{
	int 			i;

	printf(_T("%08lX "), addr);

	for (i = 0; i < cnt; i++)
	{
		printf(_T(" %02X"), buff[i]);
	}

	putchar(' ');

	for (i = 0; i < cnt; i++)
	{
		putchar((TCHAR) ((buff[i] >= ' ' && buff[i] <= '~') ? buff[i]: '.'));
	}

	putchar('\n');
}


/*--------------------------------------------------------------------------*/
/* Monitor																	*/
/*--------------------------------------------------------------------------*/
static FRESULT scan_files(char * path /* Pointer to the path name working buffer */)
{
	DIR 			dirs;
	FRESULT 		res;
	BYTE			i;
	char *			fn;

	if ((res = f_opendir(&dirs, path)) == FR_OK)
	{
		i					= strlen(path);

		while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0])
		{
			if (_FS_RPATH && Finfo.fname[0] == '.')
			{
				continue;
			}

#if _USE_LFN
			fn					= *Finfo.lfname ? Finfo.lfname: Finfo.fname;

#else

			fn					= Finfo.fname;
#endif

			if (Finfo.fattrib & AM_DIR)
			{
				acc_dirs++;
				* (path + i)		= '/';
				strcpy(path + i + 1, fn);
				res 				= scan_files(path);
				* (path + i)		= '\0';

				if (res != FR_OK)
				{
					break;
				}
			}
			else 
			{
				/* printf("%s/%s\n", path, fn); */
				acc_files++;
				acc_size			+= Finfo.fsize;
			}
		}
	}

	return res;
}



void put_rc(FRESULT rc)
{
	const TCHAR *	p	= _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")

	_T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
	_T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
	_T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0");

	uint32_t		i;

	for (i = 0; (i != (UINT) rc) && *p; i++)
	{
		while (*p++)
		{
			;
		}
	}

	printf(_T("rc=%u FR_%s\n"), (UINT) rc, p);
}


/*----------------------------------------------*/
/* Get a line from the input					*/
/*----------------------------------------------*/
void SD_get_line(char * buff, int len)
{
	TCHAR			c;
	int 			idx = 0;

	for (; ; )
	{
		c					= getchar();
		putchar(c);

		if (c == '\r')
		{
			break;
		}

		if ((c == '\b') && idx)
		{
			idx--;
		}

		if ((c >= ' ') && (idx < len - 1))
		{
			buff[idx++] 		= c;
		}
	}

	buff[idx]			= 0;

	putchar('\n');
}


#ifdef _SD_USE_INT_
extern void SD_Close_(uint32_t cardSel);


void SD0_Handler(void)
{
	unsigned int volatile isr;
	unsigned int volatile ier;

	// FMI data abort interrupt
	if (SD->FMIISR & SD_FMIISR_DTA_IE_Msk)
	{
		/* ResetAllEngine() */
		SD->FMICR			|= SD_FMICR_SW_RST_Msk;
		SD->FMIISR			= SD_FMIISR_DTA_IE_Msk;
	}

	//----- SD interrupt status
	isr 				= SD->SDISR;

	if (isr & SD_SDISR_BLKD_IF_Msk)
	{ // block down
		extern uint8_t volatile _sd_SDDataReady;

		_sd_SDDataReady 	= TRUE;
		SD->SDISR			= SD_SDISR_BLKD_IF_Msk;
	}

	if (isr & SD_SDISR_CD0_IF_Msk)
	{ // port 0 card detect

		//----- SD interrupt status
		// it is work to delay 50 times for SD_CLK = 200KHz
		{
			volatile int	i;						// delay 30 fail, 50 OK

			for (i = 0; i < 500; i++)
			{
				;									// delay to make sure got updated value from REG_SDISR.
			}

			isr 				= SD->SDISR;
		}

#ifdef _USE_DAT3_DETECT_

		if (! (isr & SD_SDISR_CDPS0_Msk))
		{
			SD0.IsCardInsert	= FALSE;
			SD_Close_(0);
		}
		else 
		{
			disk_initialize(SD_Drv);
		}

#else

		if (isr & SD_SDISR_CDPS0_Msk)
		{
			SD0.IsCardInsert	= FALSE;			// SDISR_CD_Card = 1 means card remove for GPIO mode
			SD_Close_(0);
		}
		else 
		{
			disk_initialize(SD_Drv);
		}

#endif

		SD->SDISR			= SD_SDISR_CD0_IF_Msk;
	}

	// CRC error interrupt
	if (isr & SD_SDISR_CRC_IF_Msk)
	{
		if (! (isr & SD_SDISR_CRC_16_Msk))
		{
			//printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
			// handle CRC error
		}
		else if (! (isr & SD_SDISR_CRC_7_Msk))
		{
			extern uint32_t _sd_uR3_CMD;

			if (!_sd_uR3_CMD)
			{
				//printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
				// handle CRC error
			}
		}

		SD->SDISR			= SD_SDISR_CRC_IF_Msk;	// clear interrupt flag
	}
}


void SD_IRQHandler(void)
{
	if (((SD->SDCR & SD_SDCR_SDPORT_Msk) >> (SD_SDCR_SDPORT_Pos)) == 0)
	{
		SD0_Handler();

		// } else if( ((SD->SDCR & SD_SDCR_SDPORT_Msk) >> (SD_SDCR_SDPORT_Pos)) == 1 ) {
		// SD1_Handler();
	}
}


#endif

//---------------------------------------------------------
//--- Initial SD0 multi-function GPIO pin
//
// NUC505 support 3 groups of GPIO pins and SD sockets for same one SD port.
// Please select ONLY ONE configuration from them.
// 1. SD-A socket on daughter board + default SD0_Init(). (Default)
// 2. SD-B socket on main board + short JP3 and JP4
//	  + define compile flag "SDH_GPIO_GB" in SD0_Init().
//	  (Note: this configuration conflict with UART1)
// 3. SD-C socket on main board + short JP3 and JP2
//	  + define compile flag "SDH_GPIO_GA" in SD0_Init()
//	  (Note: this configuration conflict with UART0)
//---------------------------------------------------------
void SD0_Init(void)
{
#ifdef SDH_GPIO_GA

	// The group A are GPA10~11, GPA13~15, GPB0~1
	// Conflict with UART0
	// printf("SD_Open(): Configure GPIO group A as SDH pins.\n");
	SYS->GPA_MFPH		&= (~0x77707700);
	SYS->GPA_MFPH		|= 0x44404400;
	SYS->GPA_MFPH		&= (~0x00000077);
	SYS->GPB_MFPL		|= 0x00000044;

#elif defined						SDH_GPIO_GB

	// The group B are GPB2~3, GPB5~9
	// Conflict with UART1
	// printf("SD_Open(): Configure GPIO group B as SDH pins.\n");
	SYS->GPB_MFPL		&= (~0x77707700);
	SYS->GPB_MFPL		|= 0x44404400;
	SYS->GPB_MFPH		&= (~0x00000077);
	SYS->GPB_MFPH		|= 0x00000044;

#elif defined						SDH_GPIO_G_48PIN

	// The group 48PIN are GPB0~3, GPB5~7 for NUC505 48PIN chip
	// Conflict with both UART0 and UART1
	// printf("SD_Open(): Configure special GPIO as SDH pins for 48 pins NUC505 chip.\n");
	SYS->GPB_MFPL		&= (~0x77707777);
	SYS->GPB_MFPL		|= 0x44404444;

#else // default for defined SDH_GPIO_GC

	// The group C are GPC0~2, GPC4~7
	// printf("SD_Open(): Configure GPIO group C as SDH pins.\n");
	SYS->GPC_MFPL		&= (~0x77770777);
	SYS->GPC_MFPL		|= 0x11110111;
#endif
}


/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module			   */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from	   */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.					   */
/* This function is not required in read-only cfg.		   */
unsigned long get_fattime(void)
{
	unsigned long	tmr;

	tmr = 0x00000;

	return tmr;
}


void SD_TimeOutFun( TimerID id )
{
    SD_WriteTimeOut = 1;

    SD_Process();
}

//-------------------------------------------------------------------------
// This demo program be able to access a SD card formatted in FAT file system.
//
// For NUC505 demo board NHS-NUC505-1-IN-1M11/NHS-NUC505-1-IN-1D11, please
// insert SD card to SD socket A (SD-A on daughter board) before execute program.
//-------------------------------------------------------------------------
void SD_Val_Init(void)
{
	SD_state = ( BYTE )_idle;

	printf("\n\nInitial NUC505 System Clock\n");
	printf("   CPU clock %dMHz\n", CLK_GetCPUFreq() / 1000000);
	printf("   PLL clock %dMHz\n", CLK_GetPLLClockFreq() / 1000000);

	printf("\n\nNUC505 SD FATFS TEST!\n");

	printf("   For NUC505 demo board NHS-NUC505-1-IN-1M11/NHS-NUC505-1-IN-1D11,\n");
	printf("	  please insert SD card to SD socket A (SD-A on daughter board)\n");
	printf("	  before execute program.\n");

    SD_WriteTimeOut = 0;
    SD_TimeOutTmr = TM_CreateIntTimer( TIMER_ONECYCLE, 1000, SD_TimeOutFun ); 
    // -------------------------------
    
	printf("rc=%d\n", (WORD) disk_initialize(SD_Drv));
	disk_read(SD_Drv, Buff, 2, 1);

	//f_mount(0, &FatFs[0]);  // for FATFS v0.09
	// Register work area to the default drive
	f_mount(&FatFs[0], "", 0);						// for FATFS v0.11
}


BYTE SD_WriteData(char * str)
{
	BYTE len = 0, result;

	while (str[len] != 0 && len < 64)
	{
		len++;
	}

	p1					= len;

	memcpy(Buff, str, len);

	strcpy(SD_Line, "fw");

	result				= SD_WaitCmd(SD_Line);

	p1					= 0;

	memset(Buff, 0, sizeof(Buff));

	return result;
}


BYTE SD_WaitCmd(char * ptr)
{
	FRESULT result = FR_OK;

	switch( *ptr++)
	{
		case 'f':
			switch (*ptr++)
			{
				case 'i': /* fi - Force initialized the logical drive */
					//put_rc(f_mount(0, &FatFs[0]));	// for FATFS v0.09

					result = f_mount( &FatFs[0], "", 0 );

					#if _SD_DebugMsePrint
					put_rc( result ); // for FATFS v0.11
					#endif

					break;

				case 's': /* fs - Show logical drive status */
					res = f_getfree("", (DWORD *) &p2, &fs);
					result = res;

					if (res)
					{
					    #if _SD_DebugMsePrint
						put_rc(res);
						#endif
						
						break;
					}

                    #if _SD_DebugMsePrint
					printf("FAT type = FAT%u\nBytes/Cluster = %lu\nNumber of FATs = %u\n"
					"Root DIR entries = %u\nSectors/FAT = %lu\nNumber of clusters = %lu\n"
					"FAT start (lba) = %lu\nDIR start (lba,cluster) = %lu\nData start (lba) = %lu\n\n...", 
						ft[fs->fs_type & 3], fs->csize * 512UL, fs->n_fats, 
						fs->n_rootdir, fs->fsize, fs->n_fatent - 2, 
						fs->fatbase, fs->dirbase, fs->database);
					#endif
					
					acc_size = acc_files = acc_dirs = 0;

#if _USE_LFN
					Finfo.lfname = Lfname;
					Finfo.lfsize = sizeof(Lfname);
#endif

					res = scan_files(ptr);
					result = res;

					if (res)
					{
					    #if _SD_DebugMsePrint
						put_rc(res);
						#endif
						
						break;
					}

                    #if _SD_DebugMsePrint
					printf("\r%u files, %lu bytes.\n%u folders.\n"
					"%lu KB total disk space.\n%lu KB available.\n", 
						acc_files, acc_size, acc_dirs, 
						(fs->n_fatent - 2) * (fs->csize / 2), p2 * (fs->csize / 2));
				    #endif
					break;

				case 'l': /* fl [<path>] - Directory listing */

					while( *ptr == ' ' )
					{
						ptr++;
					}

					res = f_opendir(&dir, ptr);
					result = res;

					if (res)
					{
					    #if _SD_DebugMsePrint
						put_rc(res);
						#endif
						
						break;
					}

					p1 = s1 = s2 = 0;

					for (; ; )
					{
						res = f_readdir(&dir, &Finfo);

						if ((res != FR_OK) || !Finfo.fname[0])
						{
							break;
						}

						if (Finfo.fattrib & AM_DIR)
						{
							s2++;
						}
						else 
						{
							s1++;
							p1					+= Finfo.fsize;
						}

                        #if _SD_DebugMsePrint
						printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu	%s", 
							(Finfo.fattrib & AM_DIR) ? 'D': '-', 
							(Finfo.fattrib & AM_RDO) ? 'R': '-', 
							(Finfo.fattrib & AM_HID) ? 'H': '-', 
							(Finfo.fattrib & AM_SYS) ? 'S': '-', 
							(Finfo.fattrib & AM_ARC) ? 'A': '-', 
							(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31, 
							(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, Finfo.fsize, Finfo.fname);
                        #endif
							
						if( strcmp( Finfo.fname, "LOG.TXT" ) == 0 )
						{
							SD_Tar.fsize = Finfo.fsize;
						}

#if _USE_LFN

						for (p2 = strlen(Finfo.fname); p2 < 14; p2++)
						{
							putchar(' ');
						}

                        #if _SD_DebugMsePrint
						printf("%s\n", Lfname);
						#endif

#else
                        #if _SD_DebugMsePrint
						putchar('\n');
						#endif
#endif
					}

                    #if _SD_DebugMsePrint
					printf("%4u File(s),%10lu bytes total\n%4u Dir(s)", s1, p1, s2);
					#endif

					if (f_getfree(ptr, (DWORD *) &p1, &fs) == FR_OK)
					{
					    #if _SD_DebugMsePrint
						printf(", %10lu bytes free\n", p1 * fs->csize * 512);
						#endif
					}

					break;

				case 'o': /* fo <mode> <file> - Open a file */

					if (!xatoi(&ptr, &p1))
					{
						break;
					}

					while (*ptr == ' ')
					{
						ptr++;
					}

					result = f_open( &file1, ptr, (BYTE) p1 );

                    #if _SD_DebugMsePrint
					put_rc( result );
					#endif
					
					break;

				case 'c': /* fc - Close a file */

					result = f_close(&file1);

				    #if _SD_DebugMsePrint
					put_rc( result );
					#endif
					
					break;

				case 'e': /* fe - Seek file pointer */

					if (!xatoi(&ptr, &p1))
					{
						break;
					}

					res = f_lseek(&file1, p1);
					result = res;

					#if _SD_DebugMsePrint
					put_rc(res);
					#endif

					if (res == FR_OK)
					{
					    #if _SD_DebugMsePrint
						printf("fptr=%lu(0x%lX)\n", file1.fptr, file1.fptr);
						#endif
					}
					
					break;

				case 'w': /* fw <len> <val> - write file */

					/*
						if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) )
						{
							break;
						}
					*/
					// memset(Buff, (BYTE)p2, blen);
					// memset( Buff, ( BYTE ) p2, sizeof( Buff ) );
					p2 = 0;
					Timer = system_Tick;

					while (p1)
					{
						if ( (UINT)p1 >= blen)
						{
							cnt = blen;
							p1	-= blen;
						}
						else 
						{
							cnt = p1;
							p1	= 0;
						}

						res = f_write(&file1, &Buff[SD_Output_First], cnt, &s2);
						result = res;

						if (res != FR_OK)
						{
						    #if _SD_DebugMsePrint
							put_rc(res);
							#endif
							
							break;
						}

						p2 += s2;

						if( cnt != s2 )
						{
							break;
						}
					}

					Timer2 = system_Tick;
					#if _SD_DebugMsePrint
					printf("%lu bytes written with %lu kB/sec.\n", p2, p2 / (Timer2 - Timer) / 10);
                    #endif
					break;
			}

			break;
	}
	
	return ( BYTE )result;
}


BYTE SD_FiCmd(void)
{
	return SD_WaitCmd("fi");
}


BYTE SD_FlCmd(void)
{
	return SD_WaitCmd("fl");
}


BYTE SD_FoCmd(void)
{
	return SD_WaitCmd("fo 0x02 log.txt");
}


BYTE SD_FeCmd(void)
{
	char str[32];

	strcpy( SD_Line, "fe " );

	sprintf( str, "%lu" , SD_Tar.fsize );

	strcat( SD_Line, str );

    #if _SD_DebugMsePrint
	printf( SD_Line );
    #endif

	return SD_WaitCmd(SD_Line);
}


BYTE SD_FwCmd(void)
{
	BYTE len = 1;
	BYTE result;

	if (SD_Buffer_Size != 0) // If buffer not empty
	{
		// If a new word is being output
		if( SD_Buffer_Size == SD_Input_First )
		{
			SD_Output_First 	= 0;
		}

		p1 = ( SD_Buffer_Size > len ) ?len :SD_Buffer_Size;

		result = SD_WaitCmd( "fw" );

		if (result == 0)
		{
			// Update counter
			SD_Output_First += len;

			// Decrease array size
			SD_Buffer_Size = ( SD_Buffer_Size < len ) ?0 :( SD_Buffer_Size - len );
		}

		return result;
	}
	else 
	{
		SD_Buffer_Size = 0;					// Set the array size to 0

		return 0;
	}
}


BYTE SD_FcCmd(void)
{
	return SD_WaitCmd("fc");
}


BYTE SD_Idle(void)
{
	return 0;
}


BYTE SD_WriteStr(char * str, BYTE len)
{
	if ( SD_Buffer_Size == 0 )
	{
		// If new word is entered
		SD_Input_First		= 0;
	}

	if ( ( SD_Buffer_Size + len ) < _SD_BUFFERSIZE )
	{
		memcpy( &Buff[SD_Input_First], str, len );

		SD_Buffer_Size += len; 				// Update array's size

		SD_Input_First += len; 				// Update counter

		return 0;
	}
	else 
	{
		printf("SD Buff Overflow");

		return 1;
	}
}


void SD_Process()
{
	BYTE result;

	if( SD_WriteTimeOut == 1 )
	    return;

	TM_StartTimer( SD_TimeOutTmr );

	if( SD_Buffer_Size == 0 && SD_state == _idle )
	{
		return;
	}
	else if( SD_Buffer_Size != 0 && SD_state == _idle )
	{
		SD_state = _fl;

		result = FuncArray[ SD_state ]();

        #if _SD_DebugMsePrint
		printf( SD_DebugStr[ SD_state ] );
        #endif
		
		if (result == 0)
			SD_state++;
	}
	else 
	{
		result = FuncArray[SD_state]();

        #if _SD_DebugMsePrint
		printf( SD_DebugStr[ SD_state ] );
		#endif

		if( result == 0 )
		{
			SD_state++;
		}
		else 
		{
		    #if _SD_DebugMsePrint
			put_rc( ( FRESULT )result );
			#endif
		}
	}

	TM_StopTimer( SD_TimeOutTmr );	
}


/*** (C) COPYRIGHT 2013 Nuvoton Technology Corp. ***/
