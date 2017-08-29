#ifndef _TYPEDEF_
#define _TYPEDEF_
#include <stdint.h>
#include <stddef.h>

typedef unsigned char U8;
typedef unsigned int  U16;
typedef unsigned long U32;
typedef char S8;
typedef int S16;
typedef long S32;

typedef unsigned char u8;
typedef unsigned short  u16;
typedef unsigned long u32;
typedef char s8;
typedef int s16;
typedef long s32;

#define ArrayLength(x)  (sizeof(x) / sizeof(x[0]))

#define _SUCCESS 0
#define _FAIL    1



#endif

