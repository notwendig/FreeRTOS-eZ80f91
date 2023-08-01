#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include "stdint.h"

extern uint8_t _heaptop;
extern uint8_t _heapbot;
extern size_t  _heapsize;

/* Here is a good place to include header files that are required across
your application. */
// prinf
#define PRINTF_DISABLE_SUPPORT_LONG_LONG
#define PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_DEFAULT_FLOAT_PRECISION
#define PRINTF_DISABLE_SUPPORT_LONG_LONG
#define PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#define PRINTF_DISABLE_SUPPORT_PTRDIFF_T
#define	configPRINTFTIMEOUT portMAX_DELAY //pdMS_TO_TICKS(300)

typedef    int (*progres_t)(struct uzlib_uncomp *uncomp);
int uzipp(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen, progres_t progres);
int uzip(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen);

#define pdMS_TO_TICKS( xTimeInMs ) ( TickType_t ) ((xTimeInMs) * portTICK_PERIOD_MS)

#define	configTXQUEUE_LENGTH					80
#define	configRXQUEUE_LENGTH					80
#define configCOMMAND_INT_MAX_OUTPUT_SIZE		132
#define configENABLE_FPU						0

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      50000000UL
#define configTICK_RATE_HZ                      1000UL
#define configMAX_PRIORITIES                    16U
#define configMINIMAL_STACK_SIZE                512U
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  1
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */
#define configQUEUE_REGISTRY_SIZE               10
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configSTACK_DEPTH_TYPE                  uint24_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
//#define configTOTAL_HEAP_SIZE                   (&_heaptop - &_heapbot)
#define configAPPLICATION_ALLOCATED_HEAP        1

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     1
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          1
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0


/* Software timer related definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* Interrupt nesting behaviour configuration. */
//#define configKERNEL_INTERRUPT_PRIORITY         //[dependent of processor]
//#define configMAX_SYSCALL_INTERRUPT_PRIORITY    //[dependent on processor and application]
//#define configMAX_API_CALL_INTERRUPT_PRIORITY   //[dependent on processor and application]

/* Define to trap errors during development. */
void vAssertCalled(int x, const char *file, int line);
#define configASSERT( x ) vAssertCalled((int)(x), __FILE__, __LINE__)

/* FreeRTOS MPU specific definitions. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1

/* A header file that defines trace macro can be included here. */

#define configLED5x7TICK_RATE_HZ				300
#define LED5x7_FRAMES	pdMS_TO_TICKS(  5)	// display refresch delay 
#define LED5x7_SHIFTT	pdMS_TO_TICKS( 60)	// LED5x7_FRAMES delays between shifts
#define LED5x7_CDELAY	pdMS_TO_TICKS(300)	// LED5x7_FRAMES deleys between letters
#define LED5x7_QUEUES	80					// chars on queue
#define LED5x7_PRIO								7
	
#endif /* FREERTOS_CONFIG_H */

