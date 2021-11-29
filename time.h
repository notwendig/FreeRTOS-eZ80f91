
#ifndef _TIME_H_
#define _TIME_H_
#include <stdint.h>

extern time_t iTimeZone;

typedef struct {
	uint24_t	low_HZ;
	uint24_t	high_HZ;
} sysupHZ_t;

sysupHZ_t* get_sysuptime( sysupHZ_t *upHZ);
	
typedef struct {
	uint16_t tm_year;
	uint8_t  tm_mon;
	uint8_t  tm_mday;
	uint8_t  tm_hour;
	uint8_t  tm_min;
	uint8_t  tm_sec;
} TimeStruct_t;

time_t FreeRTOS_time( time_t *t);
time_t FreeRTOS_get_secs_msec( time_t *t);
time_t FreeRTOS_set_secs_msec( time_t *uxCurrentSeconds, time_t *uxCurrentMS );
void FreeRTOS_gmtime_r( time_t *uxCurrentSeconds, TimeStruct_t *xTimeStruct );
void vStartNTPTask( uint16_t usTaskStackSize, UBaseType_t uxTaskPriority );

#endif	/* _TIME_H_ */
