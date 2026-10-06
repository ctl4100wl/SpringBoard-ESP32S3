/**
 * @file freertos_win_port.c
 * Support code for the Windows (MinGW) FreeRTOS build: clock variable and the application hooks the kernel expects.
 */
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

uint32_t SystemCoreClock = 1000000;

void vApplicationMallocFailedHook(void)
{
    fprintf(stderr, "FreeRTOS: out of heap (free: %lu bytes)\n", (unsigned long)xPortGetFreeHeapSize());
    for(;;) {
    }
}

void vApplicationIdleHook(void) {}

void vApplicationTickHook(void) {}

void vApplicationStackOverflowHook(TaskHandle_t task, char * name)
{
    (void)task;
    fprintf(stderr, "FreeRTOS: stack overflow in %s\n", name);
    for(;;) {
    }
}
