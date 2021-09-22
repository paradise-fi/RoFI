/*
 * Copyright (C) 2017 Bertold Van den Bergh. All Rights Reserved.
 * Copyright (C) 2019 Tercio Gaudencio Filho. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file    osal.c
 * @brief   OSAL module code.
 *
 * @addtogroup OSAL
 * @{
 */

#include "osal.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define LOCAL_STORAGE_QUEUE_NEXT 0
#define LOCAL_STORAGE_QUEUE_PREV 1

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

UBaseType_t uxSavedInterruptStatus;

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static bool osalThreadDequeueI(threads_queue_t *tqp, msg_t msg)
{
    if (!tqp->tail)
        return false;

    thread_t toWakeUp = tqp->tail;

    /* Lookup the last task, and pop it */
    thread_t prevTask = pvTaskGetThreadLocalStoragePointer((TaskHandle_t)toWakeUp, LOCAL_STORAGE_QUEUE_PREV);
    if (prevTask)
        vTaskSetThreadLocalStoragePointer(prevTask, LOCAL_STORAGE_QUEUE_NEXT, NULL);
    else
        tqp->head = NULL;

    tqp->tail = prevTask;

    /* Resume it */
    osalThreadResumeI((thread_reference_t *)&toWakeUp, msg);

    return true;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

msg_t osalThreadEnqueueTimeoutS(threads_queue_t *tqp, systime_t timeout)
{
    if (!timeout)
        return MSG_TIMEOUT;

    osalDbgCheck(tqp != NULL);
    osalDbgCheckClassS();

    thread_t currentTask = xGetCurrentTaskHandle();

    /* Insert in the front */
    vTaskSetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_NEXT, tqp->head);
    vTaskSetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_PREV, NULL);

    if (tqp->head)
        vTaskSetThreadLocalStoragePointer(tqp->head, LOCAL_STORAGE_QUEUE_PREV, currentTask);

    tqp->head = currentTask;
    if (!tqp->tail)
        tqp->tail = currentTask;

    msg_t msg = osalThreadSuspendTimeoutS(NULL, timeout);

    /* Remove from the queue in case of timeout */
    if (msg == MSG_TIMEOUT)
    {
        thread_t nextTask = pvTaskGetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_NEXT);
        thread_t prevTask = pvTaskGetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_PREV);

        if (nextTask)
            vTaskSetThreadLocalStoragePointer(nextTask, LOCAL_STORAGE_QUEUE_PREV, prevTask);
        else
            tqp->tail = prevTask;

        if (prevTask)
            vTaskSetThreadLocalStoragePointer(prevTask, LOCAL_STORAGE_QUEUE_NEXT, nextTask);
        else
            tqp->head = nextTask;
    }

    return msg;
}

void osalThreadDequeueAllI(threads_queue_t *tqp, msg_t msg)
{
    osalDbgCheck(tqp != NULL);
    osalDbgCheckClassI();

    while (osalThreadDequeueI(tqp, msg))
        ;
}

void osalThreadDequeueNextI(threads_queue_t *tqp, msg_t msg)
{
    osalDbgCheck(tqp != NULL);
    osalDbgCheckClassI();

    osalThreadDequeueI(tqp, msg);
}

msg_t osalThreadSuspendS(thread_reference_t *thread_reference)
{
    return osalThreadSuspendTimeoutS(thread_reference, portMAX_DELAY);
}

msg_t osalThreadSuspendTimeoutS(thread_reference_t *thread_reference, systime_t timeout)
{
    msg_t ulInterruptStatus;

    osalDbgCheckClassS();

    if (!timeout)
        return MSG_TIMEOUT;

    if (thread_reference)
        *thread_reference = (thread_reference_t)xGetCurrentTaskHandle();

    if (!xTaskNotifyWait(ULONG_MAX, ULONG_MAX, (uint32_t *)&ulInterruptStatus, timeout))
    {
        if (thread_reference)
            *thread_reference = NULL;

        return MSG_TIMEOUT;
    }

    return ulInterruptStatus;
}

eventflags_t osalEventWaitTimeoutS(event_source_t *esp, systime_t timeout)
{
    eventflags_t result = 0;
    osalDbgCheck(esp != NULL);
    osalDbgCheckClassS();

    osalThreadSuspendTimeoutS(&esp->waitThread, timeout);

    result = esp->setEvents;
    esp->setEvents = 0;

    return result;
}

void osalEventBroadcastFlagsI(event_source_t *esp, eventflags_t set)
{
    osalDbgCheck(esp != NULL);
    osalDbgCheckClassI();

    esp->setEvents |= set;
    eventflags_t localEvents = esp->setEvents;

    if (esp->eventCallback)
        esp->eventCallback(esp, localEvents);

    /* Any repeaters? */
    event_repeater_t *repeater = esp->firstRepeater;
    while (repeater)
    {
        if (localEvents & repeater->triggerEvents)
        {
            repeater->setEvents |= localEvents;
            osalEventBroadcastFlagsI(repeater->target, repeater->myEvent);
        }
        esp->setEvents &= ~repeater->triggerEvents;
        repeater = repeater->nextRepeater;
    }

    /* Wake up any waiting threads that may be waiting on remaining events */
    if (esp->setEvents)
        osalThreadResumeI(&esp->waitThread, MSG_EVENT_W);
}

#if configSUPPORT_STATIC_ALLOCATION
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCBBuffer;
    static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#if configUSE_TIMERS
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCBBuffer;
    static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = &xTimerStack[0];
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif /* configUSE_TIMERS */
#endif /* configSUPPORT_STATIC_ALLOCATION */

/** @} */
