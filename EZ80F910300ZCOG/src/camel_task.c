/* CAMEL24 shares the scheduler, but owns its dictionary and hardware stack. */
#include "FreeRTOS.h"
#include "task.h"
#include "camel_task.h"

extern StackType_t camel_task_stack[];
extern void camel_entry(void *argument);
static StaticTask_t camel_tcb;
static TaskHandle_t camel_task;

BaseType_t startCamel24(void)
{
    if(camel_task != NULL)
        return pdPASS;
    camel_task = xTaskCreateStatic(camel_entry, "CAMEL24", 1024, NULL, 2,
                                  camel_task_stack, &camel_tcb);
    return camel_task != NULL ? pdPASS : pdFAIL;
}

void camel_task_delete(void)
{
    camel_task = NULL;
    vTaskDelete(NULL);
    for(;;) { }
}
