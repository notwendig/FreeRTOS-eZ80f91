    
#ifndef _STDINT_H_
#define _STDINT_H_

/*******************************************************************************
 *
 */
 
#include <defines.h>
#include <ctype.h>

typedef signed char 	int8_t;
typedef unsigned char 	uint8_t;
typedef signed short	int16_t;
typedef unsigned short 	uint16_t;
typedef signed long 	int32_t;
typedef unsigned long 	uint32_t;
typedef signed int 		int24_t;
typedef unsigned int 	uint24_t;

typedef unsigned long 	time_t;
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

typedef unsigned int uintptr_t;

typedef struct {unsigned char b[8];	} uint64_t;
typedef struct {unsigned char w[8]; } ipv6_t;

#define SIZE_MAX		0xFFFFFFU	

#endif /* _STDINT_H_ */
