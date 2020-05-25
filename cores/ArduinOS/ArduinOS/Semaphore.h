/*
// ArduinOS
//
// ArduinOSとは, リアルタイムOS(RTOS)を理解するために, もともとあるFreeRTOSから
// 必要な機能を抜き出し, Arduinoの開発環境で使用できるようにしたものです.
//
// GNU General Public License(ver2) が適用されています.
//
// ArduinOSに関する詳しい説明は以下のページを参照してください.
// http://webviewer.php.xdomain.jp/?contentPath=./Contents/Arduino/ArduinOS/ArduinOS.html
//
*/

/*
FreeRTOS V7.4.0 - Copyright (C) 2013 Real Time Engineers Ltd.

FEATURES AND PORTS ARE ADDED TO FREERTOS ALL THE TIME.  PLEASE VISIT
http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

***************************************************************************
*                                                                       *
*    FreeRTOS tutorial books are available in pdf and paperback.        *
*    Complete, revised, and edited pdf reference manuals are also       *
*    available.                                                         *
*                                                                       *
*    Purchasing FreeRTOS documentation will not only help you, by       *
*    ensuring you get running as quickly as possible and with an        *
*    in-depth knowledge of how to use FreeRTOS, it will also help       *
*    the FreeRTOS project to continue with its mission of providing     *
*    professional grade, cross platform, de facto standard solutions    *
*    for microcontrollers - completely free of charge!                  *
*                                                                       *
*    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
*                                                                       *
*    Thank you for using FreeRTOS, and thank you for your support!      *
*                                                                       *
***************************************************************************


This file is part of the FreeRTOS distribution.

FreeRTOS is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License (version 2) as published by the
Free Software Foundation AND MODIFIED BY the FreeRTOS exception.

>>>>>>NOTE<<<<<< The modification to the GPL is included to allow you to
distribute a combined work that includes FreeRTOS without being obliged to
provide the source code for proprietary components outside of the FreeRTOS
kernel.

FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details. You should have received a copy of the GNU General Public License
and the FreeRTOS license exception along with FreeRTOS; if not itcan be
viewed here: http://www.freertos.org/a00114.html and also obtained by
writing to Real Time Engineers Ltd., contact details for whom are available
on the FreeRTOS WEB site.

1 tab == 4 spaces!

***************************************************************************
*                                                                       *
*    Having a problem?  Start by reading the FAQ "My application does   *
*    not run, what could be wrong?"                                     *
*                                                                       *
*    http://www.FreeRTOS.org/FAQHelp.html                               *
*                                                                       *
***************************************************************************


http://www.FreeRTOS.org - Documentation, books, training, latest versions,
license and Real Time Engineers Ltd. contact details.

http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
including FreeRTOS+Trace - an indispensable productivity tool, and our new
fully thread aware and reentrant UDP/IP stack.

http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
Integrity Systems, who sell the code with commercial support,
indemnification and middleware, under the OpenRTOS brand.

http://www.SafeRTOS.com - High Integrity Systems also provide a safety
engineered and independently SIL3 certified version for use in safety and
mission critical applications that require provable dependability.
*/

#ifndef ARDUINOS_SEMAPHORE_H
#define ARDUINOS_SEMAPHORE_H

#ifndef ARDUINOS_H
#error "include ArduinOS.h" must appear in source files before "include Semaphore.h"
#endif

#include "Queue.h"

typedef QueueHandle SemaphoreHandle;

#define SEM_BINARY_SEMAPHORE_QUEUE_LENGTH ((unsigned char)1U)
#define SEM_SEMAPHORE_QUEUE_ITEM_LENGTH ((unsigned char)0U)
#define SEM_GIVE_BLOCK_TIME ((PortTickType) 0U)

/*
// Macro that implements a semaphore by using the existing queue mechanism.
// The queue length is 1 as this is a binary semaphore.The data size is 0
// as we don't want to actually store any data - we just want to know if the
// queue is empty or full.
//
// This type of semaphore can be used for pure synchronisation between tasks or
// between an interrupt and a task.The semaphore need not be given back once
// obtained, so one task / interrupt can continuously 'give' the semaphore while
// another continuously 'takes' the semaphore.For this reason this type of
// semaphore does not use a priority inheritance mechanism.For an alternative
// that does use priority inheritance see SemaphoreCreateMutex().
//
// @param semaphore:
//  Handle to the created semaphore.Should be of type SemaphoreHandle.
//
// Example usage :

SemaphoreHandle semaphore;

void Task(void *parameters)
{
    // Semaphore cannot be used before a call to SemaphoreCreateBinary().
    // This is a macro so pass the variable in directly.
    SemaphoreCreateBinary(semaphore);

    if (semaphore != NULL)
    {
        // The semaphore was created successfully.
        // The semaphore can now be used.
    }
}
*/
#define SemaphoreCreateBinary(semaphore)                                                                                            \
{                                                                                                                                   \
    (semaphore) = QueueGenericCreate((unsigned PortBaseType)1, SEM_SEMAPHORE_QUEUE_ITEM_LENGTH, QUEUE_QUEUE_TYPE_BINARY_SEMAPHORE); \
    if((semaphore) != NULL)                                                                                                         \
    {                                                                                                                               \
        SemaphoreGive((semaphore));                                                                                                 \
    }                                                                                                                               \
}

/*
//
// Macro to obtain a semaphore.  The semaphore must have previously been
// created with a call to SemaphoreCreateBinary(), SemaphoreCreateMutex() or
// SemaphoreCreateCounting().
//
// @param semaphore:
//  A handle to the semaphore being taken - obtained when
//  the semaphore was created.
//
// @param blockTime:
//  The time in ticks to wait for the semaphore to become
//  available.  The macro PORT_TICK_RATE_MS can be used to convert this to a
//  real time.  A block time of zero can be used to poll the semaphore.  A block
//  time of PORT_MAX_DELAY can be used to block indefinitely (provided
//  INCLUDE_TASK_SUSPEND is set to 1 in ArduinOSConfig.h).
//
// @return pdTRUE:
//  if the semaphore was obtained.
// @return pdFALSE:
//  if blockTime expired without the semaphore becoming available.
//
// Example usage:

SemaphoreHandle semaphore = NULL;

// A task that creates a semaphore.
void Task(void *parameters)
{
    // Create the semaphore to guard  a shared resource.
    SemaphoreCreateBinary(semaphore);
}

// A task that uses the semaphore.
void AnotherTask(void *parameters)
{
    // ... Do other things.
    if (semaphore != NULL)
    {
        // See if we can obtain the semaphore.  If the semaphore is not available
        // wait 10 ticks to see if it becomes free.	
        if (SemaphoreTake(semaphore, (PortTickType)10) == PD_TRUE)
        {
            // We were able to obtain the semaphore and can now access the
            // shared resource.

            // ...

            // We have finished accessing the shared resource.  Release the 
            // semaphore.
            SemaphoreGive(semaphore);
        }
        else
        {
            // We could not obtain the semaphore and can therefore not access
            // the shared resource safely.
        }
    }
}
*/
#define SemahoreTake(semaphore, blockTime) \
    QueueGenericReceive((QueueHandle)(semaphore), NULL, (blockTime), PD_FALSE)


/*
// Macro to release a semaphore.The semaphore must have previously been
// created with a call to SemaphoreCreateBinary(), SemaphoreCreateMutex() or
// SemaphoreCreateCounting(). and obtained using SemaphoreTake().
//
// This macro must not be used from an ISR.See SemaphoreGiveFromISR() for
// an alternative which can be used from an ISR.
//
// This macro must also not be used on semaphores created using
// SemaphoreCreateRecursiveMutex().
//
// @param semaphore:
//  A handle to the semaphore being released.This is the
//  handle returned when the semaphore was created.
//
// @return pdTRUE:
//  if the semaphore was released.
// @return pdFALSE:
//  if an error occurred.
//  Semaphores are implemented using queues.An error can occur if there is
//  no space on the queue to post a message - indicating that the
//  semaphore was not first obtained correctly.
//
// Example usage :

SemaphoreHandle semaphore = NULL;

void Task(void *parameters)
{
    // Create the semaphore to guard a sharded resource.
    SemaphoreCreateBinary(semaphore);

    if (semaphore != NULL)
    {
        if (SemaphoreGive(semaphore) != PD_TRUE)
        {
            // We would expect this call to fail because we cannot give
            // a semaphore without first "taking" it!
        }

        // Obtain the semaphore - don't block if the semaphore is not
        // immediately available.
        if (SemaphoreTake(semaphore, (PortTickType)0))
        {
            // We now have the semaphore and can access the shared resource.

            // ...

            // We have finished accessing the shared resource so can free the
            // semaphore.
            if (SemaphoreGive(semaphore) != PD_TRUE)
            {
                // We would not expect this call to fail because we must have
                // obtained the semaphore to get here.
            }
        }
    }
}
*/
#define SemaphoreGive(semaphore) \
    QueueGenericSend((QueueHandle)(semaphore), NULL, SEM_GIVE_BLOCK_TIME, QUEUE_SEND_TO_BACK)

/**
//
// Macro to  release a semaphore.  The semaphore must have previously been
// created with a call to SemaphoreCreateBinary() or SemaphoreCreateCounting().
//
// Mutex type semaphores (those created using a call to xSemaphoreCreateMutex())
// must not be used with this macro.
//
// This macro can be used from an ISR.
//
// @param semaphore:
//  A handle to the semaphore being released.  This is the
//  handle returned when the semaphore was created.
//
// @param higherPriorityTaskWoken:
//  SemaphoreGiveFromISR() will set
//  higherPriorityTaskWoken to PD_TRUE if giving the semaphore caused a task
//  to unblock, and the unblocked task has a priority higher than the currently
//  running task.  If SemaphoreGiveFromISR() sets this value to PD_TRUE then
//  a context switch should be requested before the interrupt is exited.
//
// @return:
//  PD_TRUE if the semaphore was successfully given, otherwise ERR_QUEUE_FULL.
//
// Example usage:

#define LONG_TIME 0xffff
#define TICKS_TO_WAIT 10

SemaphoreHandle semaphore = NULL;

// Repetitive task.
void Task(void *paramters)
{
    for (;;)
    {
        // We want this task to run every 10 ticks of a timer.  The semaphore 
        // was created before this task was started.

        // Block waiting for the semaphore to become available.
        if (SemaphoreTake(semaphore, LONG_TIME) == pdTRUE)
        {
            // It is time to execute.

            // ...

            // We have finished our task.  Return to the top of the loop where
            // we will block on the semaphore until it is time to execute 
            // again.  Note when using the semaphore for synchronisation with an
            // ISR in this manner there is no need to 'give' the semaphore back.
        }
    }
}

// Timer ISR
void TimerISR(void * parameters)
{
    static unsigned char localTickCount = 0;
    static signed PortBaseType higherPriorityTaskWoken;

    // A timer tick has occurred.

    // ... Do other time functions.

    // Is it time for Task () to run?
    higherPriorityTaskWoken = PD_FALSE;
    localTickCount++;
    if (localTickCount >= TICKS_TO_WAIT)
    {
        // Unblock the task by releasing the semaphore.
        SemaphoreGiveFromISR(semaphore, &higherPriorityTaskWoken);

        // Reset the count so we release the semaphore again in 10 ticks time.
        localTickCount = 0;
    }

    if (higherPriorityTaskWoken != PD_FALSE)
    {
        // We can force a context switch here.  Context switching from an
        // ISR uses port specific syntax.  Check the demo task for your port
        // to find the syntax required.
    }
}
*/
#define SemaphoreGiveFromISR(semaphore, higherPriorityTaskWoken) \
    QueueGenericSendFromISR((QueueHandle)(semaphore), NULL, (higherPriorityTaskWoken), QUEUE_SEND_TO_BACK)

/**
// Macro to  take a semaphore from an ISR.  The semaphore must have
// previously been created with a call to SemaphoreCreateBinary() or
// SemaphoreCreateCounting().
//
// Mutex type semaphores (those created using a call to SemaphoreCreateMutex())
// must not be used with this macro.
//
// This macro can be used from an ISR, however taking a semaphore from an ISR
// is not a common operation.  It is likely to only be useful when taking a
// counting semaphore when an interrupt is obtaining an object from a resource
// pool (when the semaphore count indicates the number of resources available).
//
// @param semaphore:
//  A handle to the semaphore being taken.  This is the
//  handle returned when the semaphore was created.
//
// @param higherPriorityTaskWoken:
//  SemaphoreTakeFromISR() will set
//  higherPriorityTaskWoken to PD_TRUE if taking the semaphore caused a task
//  to unblock, and the unblocked task has a priority higher than the currently
//  running task.  If SemaphoreTakeFromISR() sets this value to PD_TRUE then
//  a context switch should be requested before the interrupt is exited.
//
// @return:
//  PD_TRUE if the semaphore was successfully taken, otherwise PD_FALSE
*/
#define SemaphoreTakeFromISR(semaphore, higherPriorityTaskWoken) \
    QueueReceiveFromISR((QueueHandle)(semaphore), NULL, (higherPriorityTaskWoken))

/**
// Macro that implements a mutex semaphore by using the existing queue
// mechanism.
//
// Mutexes created using this macro can be accessed using the SemaphoreTake()
// and SemaphoreGive() macros.
//
// This type of semaphore uses a priority inheritance mechanism so a task
// 'taking' a semaphore MUST ALWAYS 'give' the semaphore back once the
// semaphore it is no longer required.
//
// Mutex type semaphores cannot be used from within interrupt service routines.
//
// See SemaphoreCreateBinary() for an alternative implementation that can be
// used for pure synchronisation (where one task or interrupt always 'gives' the
// semaphore and another always 'takes' the semaphore) and from within interrupt
// service routines.
//
// @return Semaphore:
// Handle to the created mutex semaphore.  Should be of type SemaphoreHandle.
//
// Example usage:

SemaphoreHandle semaphore;

void Task( void * parameters )
{
    // Semaphore cannot be used before a call to SemaphoreCreateMutex().
    // This is a macro so pass the variable in directly.
    semaphore = SemaphoreCreateMutex();

    if(semaphore != NULL )
    {
        // The semaphore was created successfully.
        // The semaphore can now be used.
    }
}
*/
#define SemaphoreCreateMutex() QueueCreateMutex(QUEUE_QUEUE_TYPE_MUTEX)

/*
//
// Delete a semaphore. This function must be used with care. 
// For example, do not delete a mutex type semaphore if the mutex is helped by a task.
//
// @param semaphore:
//  A handle to the semaphore to be deleted.
*/
#define SemaphoreDelte(semaphore) QueueDelete((QueueHandle) (semaphore))


#endif
