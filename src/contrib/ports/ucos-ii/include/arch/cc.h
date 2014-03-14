/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : CC.h                                                    *
*                                                                         *
*   AUTHOR      : Michael Anburaj                                         *
*                 URL  : http://geocities.com/michaelanburaj/             *
*                 EMAIL: michaelanburaj@hotmail.com                       *
*                                                                         *
*   PROCESSOR   : Any                                                     *
*                                                                         *
*   TOOL-CHAIN  : Any                                                     *
*                                                                         *
*   DESCRIPTION :                                                         *
*   Architecture related header.                                          *
*                                                                         *
**************************************************************************/

#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

/* Include some files for defining library routines */
#include <string.h>
#include "little/cpu.h"
#include <stdint.h>

/* Define platform endianness */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* BYTE_ORDER */

/* Define generic types used in lwIP */
typedef uint8_t u8_t;
typedef int8_t s8_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
typedef intptr_t mem_ptr_t;

/* Compiler hints for packing structures */
#define PACK_STRUCT_FIELD(x) x __attribute__((packed))
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* prototypes for printf() and abort() */
#include <stdio.h>
#include <stdlib.h>

/* XXX Do something useful .*/
#define panic(s) {printf("%s:%d : %s", __FILE__, __LINE__, (s));while(1);}

/* Non-fatal, prints a message. Uses printf formatting. */
#define LWIP_PLATFORM_DIAG(x)   {printf x;}

/* Fatal, print message and abandon execution. Uses printf formating. The panic() function never returns. */
#define LWIP_PLATFORM_ASSERT(x)  { panic((x)); }

#define SYS_ARCH_DECL_PROTECT(x) OS_CPU_SR cpu_sr
#define SYS_ARCH_PROTECT(x)      OS_ENTER_CRITICAL()
#define SYS_ARCH_UNPROTECT(x)    OS_EXIT_CRITICAL()

#endif /* __ARCH_CC_H__ */

