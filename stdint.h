    
#ifndef _STDINT_H_
#define _STDINT_H_

/*******************************************************************************
 * THIS IS NOT A FULL stdint.h IMPLEMENTATION - It only contains the definitions
 * necessary to build the FreeRTOS code.  It is provided to allow FreeRTOS to be
 * built using compilers that do not provide their own stdint.h definition.
 *
 * To use this file:
 *
 *    1) Copy this file into the directory that contains your FreeRTOSConfig.h
 *       header file, as that directory will already be in the compilers include
 *       path.
 *
 *    2) Rename the copied file stdint.h.
 *
 */
#include <defines.h>
#include <ctype.h>

typedef signed char 	int8_t;
typedef unsigned char 	uint8_t;
typedef short 			int16_t;
typedef unsigned short 	uint16_t;
typedef long 			int32_t;
typedef unsigned long 	uint32_t;
typedef unsigned int 	uint24_t;
typedef int 			int24_t;
typedef unsigned long 	time_t;

typedef unsigned intsize_t;
typedef unsigned uintptr_t;

typedef struct {uint8_t b[8];} uint64_t;
typedef struct {uint16_t w[8];} ipv6_t;

#define SIZE_MAX		0xFFFFFF

#endif /* _STDINT_H_ */
