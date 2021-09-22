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
 * @file    osal.h
 * @brief   OSAL module header.
 *
 * @addtogroup OSAL
 * @{
 */

#ifndef OSAL_H
#define OSAL_H

#include "cmparams.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* FreeRTOS: HAL Compatibility functions */
UBaseType_t uxYieldPending(void);
xTaskHandle xGetCurrentTaskHandle(void);

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Common constants
 * @{
 */
#if !defined(FALSE)
#define FALSE /*                         */ 0
#endif

#if !defined(TRUE)
#define TRUE /*                          */ 1
#endif

#define OSAL_SUCCESS /*                  */ false
#define OSAL_FAILED /*                   */ true
/** @} */

/**
 * @name    Messages
 * @{
 */
#define MSG_OK /*                        */ (msg_t)0
#define MSG_TIMEOUT /*                   */ (msg_t) - 1
#define MSG_RESET /*                     */ (msg_t) - 2
/* FreeRTOS: Wake theads waiting for a event. */
#define MSG_EVENT_W /*                   */ (msg_t) - 3
/** @} */

/**
 * @name    Special time constants
 * @{
 */
#define TIME_IMMEDIATE /*                */ ((sysinterval_t)0)
#define TIME_INFINITE /*                 */ ((sysinterval_t)portMAX_DELAY)
/** @} */

/**
 * @name    Systick modes.
 * @{
 */
#define OSAL_ST_MODE_NONE /*             */ 0
#define OSAL_ST_MODE_PERIODIC /*         */ 1
#define OSAL_ST_MODE_FREERUNNING /*      */ 2
/** @} */

/**
 * @name    Systick parameters.
 * @{
 */
/**
 * @brief   Size in bits of the @p systick_t type.
 */
/* FreeRTOS: FreeRTOS can work with both 16 and 32-bit ticks */
#if configUSE_16_BIT_TICKS == 0
#define OSAL_ST_RESOLUTION /*            */ 32
#else
#define OSAL_ST_RESOLUTION /*            */ 16
#endif

/**
 * @brief   Required systick frequency or resolution.
 */
/* FreeRTOS: Systick rate set by FreeRTOS */
#define OSAL_ST_FREQUENCY /*             */ configTICK_RATE_HZ

/**
 * @brief   Systick mode required by the underlying OS.
 */
/* FreeRTOS: ChibiOS HAL can setup SysTick for you, but FreeRTOS already did it */
#define OSAL_ST_MODE /*                  */ OSAL_ST_MODE_NONE
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !(OSAL_ST_MODE == OSAL_ST_MODE_NONE) &&     \
    !(OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC) && \
    !(OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING)
#error "invalid OSAL_ST_MODE setting in osal.h"
#endif

#if (OSAL_ST_RESOLUTION != 16) && (OSAL_ST_RESOLUTION != 32)
#error "invalid OSAL_ST_RESOLUTION, must be 16 or 32"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a system status word.
 */
/* FreeRTOS: Stores the system state before enterring critical section */
typedef UBaseType_t syssts_t;

/**
 * @brief   Type of a message.
 */
/* FreeRTOS: Message ID */
typedef int32_t msg_t;

/**
 * @brief   Type of system time counter.
 */
/* FreeRTOS: SysTick */
typedef TickType_t systime_t;

/**
 * @brief   Type of system time interval.
 */
typedef uint32_t sysinterval_t;

/**
 * @brief   Type of realtime counter.
 */
/* FreeRTOS: Cycle time */
typedef unsigned long rtcnt_t;

/* FreeRTOS: Thread related */
typedef TaskHandle_t thread_t;

/**
 * @brief   Type of a thread reference.
 */
typedef thread_t *thread_reference_t;

/**
 * @brief   Type of an event flags mask.
 */
/* FreeRTOS: Event/Poll related */
typedef UBaseType_t eventflags_t;

typedef struct event_repeater event_repeater_t;

/**
 * @brief   Type of an event flags object.
 * @note    The content of this structure is not part of the API and should
 *          not be relied upon. Implementers may define this structure in
 *          an entirely different way.
 * @note    Retrieval and clearing of the flags are not defined in this
 *          API and are implementation-dependent.
 */
typedef struct event_source event_source_t;

struct event_source
{
  event_repeater_t *firstRepeater;
  eventflags_t setEvents;
  thread_reference_t waitThread;
  void (*eventCallback)(event_source_t *source, eventflags_t set);
};

typedef struct event_repeater
{
  event_repeater_t *nextRepeater;
  event_repeater_t *prevRepeater;
  event_source_t *source;
  event_source_t *target;
  eventflags_t triggerEvents;
  eventflags_t setEvents;
  eventflags_t myEvent;
} event_repeater_t;

/**
 * @brief   Type of a mutex.
 * @note    If the OS does not support mutexes or there is no OS then the
 *          mechanism can be simulated.
 */
typedef struct
{
  SemaphoreHandle_t handle;
  StaticSemaphore_t staticData;
} mutex_t;

/**
 * @brief   Type of a thread queue.
 * @details A thread queue is a queue of sleeping threads, queued threads
 *          can be dequeued one at time or all together.
 */
typedef struct
{
  thread_t head;
  thread_t tail;
} threads_queue_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @name    Threads abstraction macros
 */

/**
 * @brief   Static working area allocation.
 * @details This macro is used to allocate a static thread working area.
 *
 * @param[in] s         the name of the thread to be assigned to the stack array
 * @param[in] n         the stack size to be assigned to the thread
 */
#define THD_WORKING_AREA(tname, tsize)      \
  static StaticTask_t tname##Buffer; \
  static StackType_t tname##Stack[tsize];

/**
 * @brief   Thread declaration macro.
 */
#define THD_FUNCTION(tname, tparam) static __attribute__((noreturn)) void tname(void *tparam)

/** @} */

/**
 * @name    Debug related macros
 * @{
 */
/**
 * @brief   Condition assertion.
 * @details If the condition check fails then the OSAL panics with a
 *          message and halts.
 * @note    The condition is tested only if the @p OSAL_ENABLE_ASSERTIONS
 *          switch is enabled.
 * @note    The remark string is not currently used except for putting a
 *          comment in the code about the assertion.
 *
 * @param[in] c         the condition to be verified to be true
 * @param[in] remark    a remark string
 *
 * @api
 */
#if (configASSERT_DEFINED == TRUE)
#define osalDbgAssert(c, remark) \
  {                              \
    if (!(c))                    \
    {                            \
      osalSysHalt(remark);       \
    }                            \
  }
#else
#define osalDbgAssert(c, remark)
#endif

/**
 * @brief   Function parameters check.
 * @details If the condition check fails then the OSAL panics and halts.
 * @note    The condition is tested only if the @p OSAL_ENABLE_CHECKS switch
 *          is enabled.
 *
 * @param[in] c         the condition to be verified to be true
 *
 * @api
 */
#define osalDbgCheck(c) osalDbgAssert(c, __func__)

/**
 * @brief   I-Class state check.
 */
#if (configASSERT_DEFINED == TRUE)
#define osalDbgCheckClassI() osalDbgAssert(xPortIsCriticalSection(), "not in critical section");
#else
#define osalDbgCheckClassI()
#endif

/**
 * @brief   S-Class state check.
 */
#if (configASSERT_DEFINED == TRUE)
#define osalDbgCheckClassS()                                  \
  do                                                          \
  {                                                           \
    osalDbgCheckClassI();                                     \
    osalDbgAssert(!xPortIsInsideInterrupt(), "in interrupt"); \
  } while (0);
#else
#define osalDbgCheckClassS()
#endif

/** @} */

/**
 * @name    IRQ service routines wrappers
 * @{
 */
/**
 * @brief   Priority level verification macro.
 */
/* FreeRTOS: FreeRTOS has a runtime check for invalid priorities, making this check redudant */
#define OSAL_IRQ_IS_VALID_PRIORITY(n) TRUE

/**
 * @brief   IRQ prologue code.
 * @details This macro must be inserted at the start of all IRQ handlers.
 */
#define OSAL_IRQ_PROLOGUE()

/**
 * @brief   IRQ epilogue code.
 * @details This macro must be inserted at the end of all IRQ handlers.
 */
/* FreeRTOS: At the end of the interrupt handler we need to check if we need to yield */
#define OSAL_IRQ_EPILOGUE()               \
  {                                       \
    portYIELD_FROM_ISR(uxYieldPending()); \
  }

/**
 * @brief   IRQ handler function declaration.
 * @details This macro hides the details of an ISR function declaration.
 *
 * @param[in] id        a vector name as defined in @p vectors.s
 */
#define OSAL_IRQ_HANDLER(id) void id(void)
/** @} */

/**
 * @name    Time conversion utilities
 * @{
 */
#define OSAL_T2I(t, hz, div) ((systime_t)(((uint32_t)(hz) * (uint32_t)(t) + (uint32_t)(div - 1)) / (uint32_t)(div)))

/**
 * @brief   Seconds to system ticks.
 * @details Converts from seconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] secs      number of seconds
 * @return              The number of ticks.
 *
 * @api
 */
#define OSAL_S2I(secs) OSAL_T2I(secs, configTICK_RATE_HZ, 1)

/**
 * @brief   Milliseconds to system ticks.
 * @details Converts from milliseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] msecs     number of milliseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define OSAL_MS2I(msecs) OSAL_T2I(msecs, configTICK_RATE_HZ, 1000)

/**
 * @brief   Microseconds to system ticks.
 * @details Converts from microseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] usecs     number of microseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define OSAL_US2I(usecs) OSAL_T2I(usecs, configTICK_RATE_HZ, 1000000)
/** @} */

/**
 * @name    Time conversion utilities for the realtime counter
 * @{
 */
/**
 * @brief   Seconds to realtime counter.
 * @details Converts from seconds to realtime counter cycles.
 * @note    The macro assumes that @p freq >= @p 1.
 *
 * @param[in] freq      clock frequency, in Hz, of the realtime counter
 * @param[in] sec       number of seconds
 * @return              The number of cycles.
 *
 * @api
 */
#define OSAL_S2RTC(freq, sec) S2RTC(freq, sec)

/**
 * @brief   Milliseconds to realtime counter.
 * @details Converts from milliseconds to realtime counter cycles.
 * @note    The result is rounded upward to the next millisecond boundary.
 * @note    The macro assumes that @p freq >= @p 1000.
 *
 * @param[in] freq      clock frequency, in Hz, of the realtime counter
 * @param[in] msec      number of milliseconds
 * @return              The number of cycles.
 *
 * @api
 */
#define OSAL_MS2RTC(freq, msec) MS2RTC(freq, msec)

/**
 * @brief   Microseconds to realtime counter.
 * @details Converts from microseconds to realtime counter cycles.
 * @note    The result is rounded upward to the next microsecond boundary.
 * @note    The macro assumes that @p freq >= @p 1000000.
 *
 * @param[in] freq      clock frequency, in Hz, of the realtime counter
 * @param[in] usec      number of microseconds
 * @return              The number of cycles.
 *
 * @api
 */
#define OSAL_US2RTC(freq, usec) US2RTC(freq, usec)
/** @} */

/**
 * @name    Sleep macros using absolute time
 * @{
 */
/**
 * @brief   Delays the invoking thread for the specified number of seconds.
 * @note    The specified time is rounded up to a value allowed by the real
 *          system tick clock.
 * @note    The maximum specifiable value is implementation dependent.
 *
 * @param[in] secs      time in seconds, must be different from zero
 *
 * @api
 */
#define osalThreadSleepSeconds(secs) osalThreadSleep(OSAL_S2I(secs))

/**
 * @brief   Delays the invoking thread for the specified number of
 *          milliseconds.
 * @note    The specified time is rounded up to a value allowed by the real
 *          system tick clock.
 * @note    The maximum specifiable value is implementation dependent.
 *
 * @param[in] msecs     time in milliseconds, must be different from zero
 *
 * @api
 */
#define osalThreadSleepMilliseconds(msecs) osalThreadSleep(OSAL_MS2I(msecs))

/**
 * @brief   Delays the invoking thread for the specified number of
 *          microseconds.
 * @note    The specified time is rounded up to a value allowed by the real
 *          system tick clock.
 * @note    The maximum specifiable value is implementation dependent.
 *
 * @param[in] usecs     time in microseconds, must be different from zero
 *
 * @api
 */
#define osalThreadSleepMicroseconds(usecs) osalThreadSleep(OSAL_US2I(usecs))
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   OSAL module initialization.
 *
 * @api
 */
static inline void osalInit(void)
{
}

/**
 * @brief   Disables interrupts globally.
 *
 * @special
 */
#define osalSysDisable vTaskEndScheduler

/**
 * @brief   System halt with error message.
 *
 * @param[in] reason    the halt message pointer
 *
 * @api
 */
static inline void osalSysHalt(const char *reason)
{

  do
  {
    osalSysDisable();
#if (configASSERT_DEFINED == TRUE)
    vApplicationAssertHook(__FILE__, __LINE__, reason);
#endif
  } while (1);
}

/**
 * @brief   Enables interrupts globally.
 *
 * @special
 */
#define osalSysEnable vTaskStartScheduler

/**
 * @brief   Enters a critical zone from thread context.
 * @note    This function cannot be used for reentrant critical zones.
 *
 * @special
 */
#define osalSysLock taskENTER_CRITICAL

static void osalOsRescheduleS(void);
/**
 * @brief   Leaves a critical zone from thread context.
 * @note    This function cannot be used for reentrant critical zones.
 *
 * @special
 */
static inline void osalSysUnlock(void)
{

  osalOsRescheduleS();
  taskEXIT_CRITICAL();
}

extern UBaseType_t uxSavedInterruptStatus;
/**
 * @brief   Enters a critical zone from ISR context.
 * @note    This function cannot be used for reentrant critical zones.
 *
 * @special
 */
static inline void osalSysLockFromISR(void)
{

  uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
}

/**
 * @brief   Leaves a critical zone from ISR context.
 * @note    This function cannot be used for reentrant critical zones.
 *
 * @special
 */
static inline void osalSysUnlockFromISR(void)
{

  taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

/**
 * @brief   Returns the execution status and enters a critical zone.
 * @details This functions enters into a critical zone and can be called
 *          from any context. Because its flexibility it is less efficient
 *          than @p chSysLock() which is preferable when the calling context
 *          is known.
 * @post    The system is in a critical zone.
 *
 * @return              The previous system status, the encoding of this
 *                      status word is architecture-dependent and opaque.
 *
 * @xclass
 */
#define osalSysGetStatusAndLockX taskENTER_CRITICAL_FROM_ISR

/**
 * @brief   Restores the specified execution status and leaves a critical zone.
 * @note    A call to @p chSchRescheduleS() is automatically performed
 *          if exiting the critical zone and if not in ISR context.
 *
 * @param[in] sts       the system status to be restored.
 *
 * @xclass
 */
#define osalSysRestoreStatusX(state) taskEXIT_CRITICAL_FROM_ISR(state)

/**
 * @brief   Polled delay.
 * @note    The real delay is always few cycles in excess of the specified
 *          value.
 *
 * @param[in] cycles    number of cycles
 *
 * @xclass
 */
static inline void osalSysPolledDelayX(rtcnt_t cycles)
{

  unsigned long i = 0;
  /* In my case, each loop takes 8 cycles. This depends on many things, such
  * as the number of flash wait states. You need to adjust this if you need
  * an accurate polled delay (rarely needed) */
  cycles /= configPORT_BUSY_DELAY_SCALE;

  __asm volatile(
      "loop%=:                     \n"
      "       cmp %0, %1           \n"
      "       beq done%=           \n"
      "       adds %0, 1           \n"
      "       b loop%=             \n"
      "done%=:                     \n"
      : "+r"(i)
      : "r"(cycles));
}

/**
 * @brief   Checks if a reschedule is required and performs it.
 * @note    I-Class functions invoked from thread context must not reschedule
 *          by themselves, an explicit reschedule using this function is
 *          required in this scenario.
 * @note    Not implemented in this simplified OSAL.
 *
 * @sclass
 */
static inline void osalOsRescheduleS(void)
{

  osalDbgCheckClassS();

  if (uxYieldPending())
    taskYIELD();
}

/**
 * @brief   Current system time.
 * @details Returns the number of system ticks since the @p osalInit()
 *          invocation.
 * @note    The counter can reach its maximum and then restart from zero.
 * @note    This function can be called from any context but its atomicity
 *          is not guaranteed on architectures whose word size is less than
 *          @p systime_t size.
 *
 * @return              The system time in ticks.
 *
 * @xclass
 */
#define osalOsGetSystemTimeX xTaskGetTickCountFromISR

/**
 * @brief   Adds an interval to a system time returning a system time.
 *
 * @param[in] systime   base system time
 * @param[in] interval  interval to be added
 * @return              The new system time.
 *
 * @xclass
 */
static inline systime_t osalTimeAddX(systime_t systime, sysinterval_t interval)
{

  return ((systime_t)(systime) + (systime_t)(interval));
}

/**
 * @brief   Subtracts two system times returning an interval.
 *
 * @param[in] start     first system time
 * @param[in] end       second system time
 * @return              The interval representing the time difference.
 *
 * @xclass
 */
static inline sysinterval_t osalTimeDiffX(systime_t start, systime_t end)
{

  return ((sysinterval_t)((systime_t)((systime_t)(end) - (systime_t)(start))));
}

/**
 * @brief   Checks if the specified time is within the specified time window.
 * @note    When start==end then the function returns always true because the
 *          whole time range is specified.
 * @note    This function can be called from any context.
 *
 * @param[in] time      the time to be verified
 * @param[in] start     the start of the time window (inclusive)
 * @param[in] end       the end of the time window (non inclusive)
 * @retval true         current time within the specified time window.
 * @retval false        current time not within the specified time window.
 *
 * @xclass
 */
static inline bool osalTimeIsInRangeX(systime_t time, systime_t start, systime_t end)
{

  return ((bool)((systime_t)((systime_t)(time) - (systime_t)(start)) <
                 (systime_t)((systime_t)(end) - (systime_t)(start))));
}

/**
 * @brief   Suspends the invoking thread for the specified time.
 *
 * @param[in] delay     the delay in system ticks, the special values are
 *                      handled as follow:
 *                      - @a TIME_INFINITE is allowed but interpreted as a
 *                        normal time specification.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 *
 * @sclass
 */
#define osalThreadSleepS(time) vTaskDelay(time)

/**
 * @brief   Suspends the invoking thread for the specified time.
 *
 * @param[in] delay     the delay in system ticks, the special values are
 *                      handled as follow:
 *                      - @a TIME_INFINITE is allowed but interpreted as a
 *                        normal time specification.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 *
 * @api
 */
#define osalThreadSleep(time) vTaskDelay(time)

/**
 * @brief   Sends the current thread sleeping and sets a reference variable.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @return              The wake up message.
 *
 * @sclass
 */
msg_t osalThreadSuspendS(thread_reference_t *trp);

/**
 * @brief   Sends the current thread sleeping and sets a reference variable.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] timeout   the timeout in system ticks, the special values are
 *                      handled as follow:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE the thread is not enqueued and
 *                        the function returns @p MSG_TIMEOUT as if a timeout
 *                        occurred.
 * @return              The wake up message.
 * @retval MSG_TIMEOUT  if the operation timed out.
 *
 * @sclass
 */
msg_t osalThreadSuspendTimeoutS(thread_reference_t *trp,
                                sysinterval_t timeout);

/**
 * @brief   Wakes up a thread waiting on a thread reference object.
 * @note    This function must not reschedule because it can be called from
 *          ISR context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] msg       the message code
 *
 * @iclass
 */
static inline void osalThreadResumeI(thread_reference_t *trp, msg_t msg)
{

  osalDbgCheckClassI();

  if (*trp)
  {
    xTaskNotifyFromISR((TaskHandle_t)*trp, msg, eSetValueWithOverwrite, NULL);
    *trp = NULL;
  }
}

/**
 * @brief   Wakes up a thread waiting on a thread reference object.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] msg       the message code
 *
 * @iclass
 */
static inline void osalThreadResumeS(thread_reference_t *trp, msg_t msg)
{

  osalDbgCheckClassS();

  if (*trp)
  {
    xTaskNotify((TaskHandle_t)*trp, msg, eSetValueWithOverwrite);
    *trp = NULL;
  }
}

/**
 * @brief   Initializes a threads queue object.
 *
 * @param[out] tqp      pointer to the threads queue object
 *
 * @init
 */
static inline void osalThreadQueueObjectInit(threads_queue_t *tqp)
{

  osalDbgCheck(tqp != NULL);

  tqp->head = NULL;
  tqp->tail = NULL;
}

/**
 * @brief   Enqueues the caller thread.
 * @details The caller thread is enqueued and put to sleep until it is
 *          dequeued or the specified timeouts expires.
 *
 * @param[in] tqp       pointer to the threads queue object
 * @param[in] timeout   the timeout in system ticks, the special values are
 *                      handled as follow:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE the thread is not enqueued and
 *                        the function returns @p MSG_TIMEOUT as if a timeout
 *                        occurred.
 *                      .
 * @return              The message from @p osalQueueWakeupOneI() or
 *                      @p osalQueueWakeupAllI() functions.
 * @retval MSG_TIMEOUT  if the thread has not been dequeued within the
 *                      specified timeout or if the function has been
 *                      invoked with @p TIME_IMMEDIATE as timeout
 *                      specification.
 *
 * @sclass
 */
msg_t osalThreadEnqueueTimeoutS(threads_queue_t *tqp,
                                sysinterval_t timeout);

/**
 * @brief   Dequeues and wakes up one thread from the queue, if any.
 *
 * @param[in] tqp       pointer to the threads queue object
 * @param[in] msg       the message code
 *
 * @iclass
 */
void osalThreadDequeueNextI(threads_queue_t *tqp, msg_t msg);

/**
 * @brief   Dequeues and wakes up all threads from the queue.
 *
 * @param[in] tqp       pointer to the threads queue object
 * @param[in] msg       the message code
 *
 * @iclass
 */
void osalThreadDequeueAllI(threads_queue_t *tqp, msg_t msg);

static inline void osalEventObjectInit(event_source_t *esp)
{

  osalDbgCheck(esp != NULL);

  esp->setEvents = 0;
  esp->firstRepeater = NULL;
  esp->waitThread = NULL;
}

/**
 * @brief   Add flags to an event source object.
 *
 * @param[in] esp       pointer to the event flags object
 * @param[in] flags     flags to be ORed to the flags mask
 *
 * @iclass
 */
void osalEventBroadcastFlagsI(event_source_t *esp, eventflags_t flags);

/**
 * @brief   Add flags to an event source object.
 *
 * @param[in] esp       pointer to the event flags object
 * @param[in] flags     flags to be ORed to the flags mask
 *
 * @iclass
 */
static inline void osalEventBroadcastFlags(event_source_t *esp,
                                           eventflags_t flags)
{

  osalDbgCheck(esp != NULL);

  osalSysLock();
  osalEventBroadcastFlagsI(esp, flags);
  osalSysUnlock();
}

/**
 * @brief   Initializes s @p mutex_t object.
 *
 * @param[out] mp       pointer to the @p mutex_t object
 *
 * @init
 */
static inline void osalMutexObjectInit(mutex_t *mp)
{

  osalDbgCheck(mp != NULL);

  mp->handle = xSemaphoreCreateMutexStatic(&mp->staticData);
}

/**
 * @brief   Locks the specified mutex.
 * @post    The mutex is locked and inserted in the per-thread stack of owned
 *          mutexes.
 *
 * @param[in,out] mp    pointer to the @p mutex_t object
 *
 * @api
 */
static inline void osalMutexLock(mutex_t *mp)
{

  osalDbgCheck(mp != NULL);

  xSemaphoreTake(mp->handle, portMAX_DELAY);
}

/**
 * @brief   Unlocks the specified mutex.
 * @note    The HAL guarantees to release mutex in reverse lock order. The
 *          mutex being unlocked is guaranteed to be the last locked mutex
 *          by the invoking thread.
 *          The implementation can rely on this behavior and eventually
 *          ignore the @p mp parameter which is supplied in order to support
 *          those OSes not supporting a stack of the owned mutexes.
 *
 * @param[in,out] mp    pointer to the @p mutex_t object
 *
 * @api
 */
static inline void osalMutexUnlock(mutex_t *mp)
{

  osalDbgCheck(mp != NULL);

  xSemaphoreGive(mp->handle);
}

#endif /* OSAL_H */

/** @} */
