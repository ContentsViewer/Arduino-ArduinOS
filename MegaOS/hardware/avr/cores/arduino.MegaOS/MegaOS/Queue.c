/*
// MegaOS
//
// MegaOSとは, リアルタイムOS(RTOS)を理解するために, もともとあるFreeRTOSから
// 必要な機能を抜き出し, Arduinoの開発環境で使用できるようにしたものです.
//
// GNU General Public License(ver2) が適用されています.
//
// MegaOSに関する詳しい説明は以下のページを参照してください.
// http://webviewer.php.xdomain.jp/?contentPath=./Contents/Arduino/MegaOS/MegaOS.html
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
#include <stdlib.h>
#include <string.h>

#include "MegaOS.h"
#include "Task.h"
#include "Queue.h"


#define QUEUE_UNLOCKED ((signed PortBaseType)-1)
#define QUEUE_LOCKED_UNMODIFIED ((signed PortBaseType)0)

#define mutexHolder tail
#define isMutex head
#define QUEUE_QUEUE_IS_MUTEX NULL

typedef struct 
{
    // Points to the beginning of the queue storage area.
    signed char *head;

    // Points to the byte at the end of the queue storage area.
    // Once more byte is allocated than necessary to store the queue items, this is used as a marker.
    signed char *tail;
    
    // Points to the free next place in the storage area.
    signed char *writeTo;

    // Points to the last place that a queued item was read from.
    signed char *readFrom;

    // List of tasks that are blocked waiting to post onto this queue. Stored in priority order.
    List tasksWaitingToSend;

    // List of tasks that are blocked waiting to read from this queue. Stored in priority order.
    List tasksWaitingToReceive;

    // The number of items currently in the queue.
    volatile unsigned PortBaseType messagesWaiting;

    // The length of the queue defined as the number of items it will hold, not the number of bytes.
    unsigned PortBaseType length;

    //The size of each items that the queue will holds.
    unsigned PortBaseType itemSize;

    // Stores the number of items received from the queue (removed from the queue)
    // while the queue was locked. Set to QUEUE_UNLOCKED when the queue is not locked.
    volatile signed PortBaseType rxLock;

    // Stores the number of items transmitted to the queue (added to the queue)
    // while the queue was locked. Set to QUEUE_UNLOCKED when the queue is not locked.
    volatile signed PortBaseType txLock;
}Queue;

/*
// Unlocks a queue locked by a call to LockQueue.  Locking a queue does not
// prevent an ISR from adding or removing items to the queue, but does prevent
// an ISR from removing tasks from the queue event lists.  If an ISR finds a
// queue is locked it will instead increment the appropriate queue lock count
// to indicate that a task may require unblocking.  When the queue in unlocked
// these lock counts are inspected, and the appropriate action taken.
*/
static void UnlockQueue(Queue *queue);

/*
// Uses a critical section to determine if there is any data in a queue.
//
// @return pdTRUE:
//  if the queue contains no items, otherwise pdFALSE.
*/
static signed PortBaseType IsQueueEmpty(const Queue *queue);

/*
// Uses a critical section to determine if there is any space in a queue.
//
// @return pdTRUE:
//  if there is no space, otherwise pdFALSE;
*/
static signed PortBaseType IsQueueFull(const Queue *queue);

/*
// Copies an item into the queue, either at the front of the queue or the
// back of the queue.
*/
static void CopyDataToQueue(Queue *queue, const void *itemToQueue, PortBaseType position);

/*
// Copies an item out of a queue.
*/
static void CopyDataFromQueue(Queue *const queue, const void *buffer);


//
// Macro to mark a queue as locked. Locking a queue prevents an ISR from
// accessing the queue event lists.
//
#define LockQueue(queue)                                \
    TaskEnterCritical();                                \
    {                                                   \
        if((queue)->rxLock == QUEUE_UNLOCKED)           \
        {                                               \
            (queue)->rxLock = QUEUE_LOCKED_UNMODIFIED;  \
        }                                               \
        if((queue)->txLock == QUEUE_UNLOCKED)           \
        {                                               \
            (queue)->txLock = QUEUE_LOCKED_UNMODIFIED;  \
        }                                               \
    }                                                   \
    TaskExitCritical();

PortBaseType QueueGenericReset(QueueHandle queueToReset, PortBaseType newQueue)
{
    Queue *queue;

    queue = (Queue *)queueToReset;

    TaskEnterCritical();
    {
        queue->tail = queue->head + (queue->length * queue->itemSize);
        queue->messagesWaiting = (unsigned PortBaseType)0U;
        queue->writeTo = queue->head;
        queue->readFrom = queue->head + ((queue->length - (unsigned PortBaseType)1U) * queue->itemSize);
        queue->rxLock = QUEUE_UNLOCKED;
        queue->txLock = QUEUE_UNLOCKED;

        if (newQueue == PD_FALSE)
        {
            // If there are tasks blocked waiting to read from the queue, then
            // the tasks will remain blocked as after this function exits the queue
            // will still be empty.  If there are tasks blocked waiting to	write to
            // the queue, then one should be unblocked as after this function exits
            // it will be possible to write to it. 
            if (ListListIsEmpty(&(queue->tasksWaitingToSend)) == PD_FALSE)
            {
                if (TaskRemoveFromEventList(&(queue->tasksWaitingToSend)) == PD_TRUE)
                {
                    PortYieldWithinAPI();
                }
            }
        }
        else
        {
            // Ensure the event queues start in the correct state.
            ListInitialise(&(queue->tasksWaitingToSend));
            ListInitialise(&(queue->tasksWaitingToReceive));
        }
    }
    TaskExitCritical();

    // A value is returned for calling semantic consistency with previous versions.
    return PD_PASS;
}

QueueHandle QueueGenericCreate(unsigned PortBaseType queueLength, unsigned PortBaseType itemSize, unsigned char queueType)
{
    Queue *newQueue;
    size_t queueSizeInBytes;
    QueueHandle ret = NULL;

    // Remove compiler warnings about unused parameters.
    (void)queueType;

    // Allocate the new queue structure.
    if (queueLength > (unsigned PortBaseType)0)
    {
        newQueue = (Queue *)PortMalloc(sizeof(Queue));
        if (newQueue != NULL)
        {
            // Crate the list of pointer to queue items. The queue is one byte
            // longer than asked for to make wrap checking easier/faster.
            queueSizeInBytes = (size_t)(queueLength * itemSize) + (size_t)1;

            newQueue->head = (signed char *)PortMalloc(queueSizeInBytes);
            if (newQueue->head != NULL)
            {
                newQueue->length = queueLength;
                newQueue->itemSize = itemSize;
                QueueGenericReset(newQueue, PD_TRUE);

                TraceQueueCreate(newQueue);
                ret = newQueue;
            }
            else
            {
                TraceQueueCreateFailed(queueType);
                PortFree(newQueue);
            }
        }
    }

    return ret;
}

#if (CONFIG_USE_MUTEXES == 1)
QueueHandle QueueCreateMutex(unsigned char queueType)
{
    Queue *newQueue;

    // Prevent compiler warnings about used parameters
    (void)queueType;

    // Allocate the new queue structure.
    newQueue = (Queue *)PortMalloc(sizeof(Queue));
    if (newQueue != NULL)
    {
        // Information required for priority inheritance.
        newQueue->mutexHolder = NULL;
        newQueue->isMutex = QUEUE_QUEUE_IS_MUTEX;

        // Queues used as a mutex no data is actually copied into or out of the queue.
        newQueue->writeTo = NULL;
        newQueue->readFrom = NULL;

        // Each mutex has a length of 1 (like a binary semaphore) and
        // an item size of 0 as nothing is actually copied into or out
        // of the mutex.
        newQueue->messagesWaiting = (unsigned PortBaseType)0U;
        newQueue->length = (unsigned PortBaseType)1U;
        newQueue->itemSize = (unsigned PortBaseType)0U;
        newQueue->rxLock = QUEUE_UNLOCKED;
        newQueue->txLock = QUEUE_UNLOCKED;

        // Ensure the event queues start with the correct state.
        ListInitialise(&(newQueue->tasksWaitingToSend));
        ListInitialise(&(newQueue->tasksWaitingToReceive));

        TraceCreateMutex(newQueue);

        // Start with the semaphore in the expected state.
        QueueGenericSend(newQueue, NULL, (PortTickType)0U, QUEUE_SEND_TO_BACK);
    }
    else
    {
        TraceCreateMutexFailed();
    }

    return newQueue;
} 
#endif

signed PortBaseType QueueGenericSend(QueueHandle queueTo, const void * const itemToQueue, 
    PortTickType ticksToWait, PortBaseType copyPosition)
{
    signed PortBaseType entryTimeSet = PD_FALSE;
    TimeOutType timeOut;
    Queue *queue;

    queue = (Queue *)queueTo;

    // This function relaxes the coding standard somewhat to allow return
    // statements within the function itself.  This is done in the interest
    // of execution time efficiency.
    for (;;)
    {
        TaskEnterCritical();
        {
            // キューに空きがあるか? 
            // Is there room on the queue now?  To be running we must be
            // the highest priority task wanting to access the queue.
            if (queue->messagesWaiting < queue->length)
            {
                TraceQueueSend(queue);
                CopyDataToQueue(queue, itemToQueue, copyPosition);

                // Queueからアイテムを取得しようと待機しているタスクを
                // 待機解除にする.
                // If there was a task waiting for data to arrive on the
                // queue then unblock it now.
                if (ListListIsEmpty(&(queue->tasksWaitingToReceive)) == PD_FALSE)
                {
                    if (TaskRemoveFromEventList(&(queue->tasksWaitingToReceive)) == PD_TRUE)
                    {
                        // The unblocked task has a priority higher than
                        // our own so yield immediately.  Yes it is ok to do
                        // this from within the critical section - the kernel
                        // takes care of that.
                        PortYieldWithinAPI();
                    }
                }

                TaskExitCritical();

                // Return to the original privilage level before exiting the function.
                return PD_PASS;
            }
            else
            {
                if (ticksToWait == (PortTickType)0)
                {
                    // The queue was full and no block time is specified
                    // (or the block time has expired) so leave now.
                    TaskExitCritical();

                    // Return to the original privilage level before exiting
                    // the function.
                    TraceQueueSendFailed(queue);
                    return ERR_QUEUE_FULL;
                }
                else if (entryTimeSet == PD_FALSE)
                {
                    // The queue was full and a block time was specified so
                    // configure the timeout structure.
                    TaskSetTimeOutState(&timeOut);
                    entryTimeSet = PD_TRUE;
                }
            }
        }
        TaskExitCritical();

        // Interrupts and other tasks can send to and receive from the queue
        // now the critical section has been exited.

        TaskSuspendAll();
        LockQueue(queue);

        // Update the timeout state to see if it has expired yet.
        if (TaskCheckForTimeOut(&timeOut, &ticksToWait) == PD_FALSE)
        {
            if (IsQueueFull(queue) != PD_FALSE)
            {
                TraceBlockingOnQueueSend(queue);
                TaskPlaceOnEventList(&(queue->tasksWaitingToSend), ticksToWait);

                // Unlocking the queue means queue events can effect the
                // event list.  It is possible	that interrupts occurring now
                // remove this task from the event	list again - but as the
                // scheduler is suspended the task will go onto the pending
                // ready last instead of the actual ready list.
                UnlockQueue(queue);

                // Resuming the scheduler will move tasks from the pending
                // ready list into the ready list - so it is feasible that this
                // task is already in a ready list before it yields - in which
                // case the yield will not cause a context switch unless there
                // is also a higher priority task in the pending ready list.
                if (TaskResumeAll() == PD_FALSE)
                {
                    PortYieldWithinAPI();
                }
            }
            else
            {
                // Try again.
                UnlockQueue(queue);
                (void)TaskResumeAll();
            }
        }
        else
        {
            // The timeout has expired.
            UnlockQueue(queue);
            (void)TaskResumeAll();

            // Return to the original privilage level before exiting the function.
            TraceQueueSendFailed(queue);
            return ERR_QUEUE_FULL;
        }
    }
}

signed PortBaseType QueueGenericSendFromISR(QueueHandle queueTo, const void * const itemToQueue,
    signed PortBaseType *higherPriorityTaskWoken, PortBaseType copyPosition)
{
    signed PortBaseType ret;
    
    Queue *queue;

    queue = (Queue *)queueTo;

    // Similar to QueueGenericSend, except we don't block if there is no room
    //in the queue.  Also we don't directly wake a task that was blocked on a
    //queue read, instead we return a flag to say whether a context switch is
    //required or not (i.e. has a task with a higher priority than us been woken
    // by this	post).

    if (queue->messagesWaiting < queue->length)
    {
        TraceQueueSendFromISR(queue);

        CopyDataToQueue(queue, itemToQueue, copyPosition);

        // If the queue is locked we do not alter the event list.  This will
        // be done when the queue is unlocked later.
        if (queue->rxLock == QUEUE_UNLOCKED)
        {
            if (ListListIsEmpty(&(queue->tasksWaitingToReceive)) == PD_FALSE)
            {
                if (TaskRemoveFromEventList(&(queue->tasksWaitingToReceive)) != PD_FALSE)
                {
                    // The task waiting has a higher priority so record that a 
                    // context switch is requred.
                    if (higherPriorityTaskWoken != NULL)
                    {
                        higherPriorityTaskWoken = PD_TRUE;
                    }
                }
            }
        }
        else
        {
            // Increment the lock count so the task that unlocks the queue
            // knows that data was posted while it was locked.
            ++(queue->rxLock);
        }
        ret = PD_PASS;
    }
    else
    {
        TraceQueueSendFromISRFailed(queue);
        ret = ERR_QUEUE_FULL;
    }

    return ret;
}

signed PortBaseType QueueGenericReceive(QueueHandle queueFrom, void * const buffer,
    PortTickType ticksToWait, PortBaseType justPeeking)
{
    signed PortBaseType entryTimeSet = PD_FALSE;
    TimeOutType timeOut;
    signed char *originalReadPositon;
    Queue *queue;

    queue = (Queue *)queueFrom;

    // This function relaxes the coding standard somewhat to allow return
    // statements within the function itself.  This is done in the interest
    // of execution time efficiency.

    for (;;)
    {
        TaskEnterCritical();
        {
            // Queue内にデータがあるかどうか? 
            // Is there data in the queue now? To be running we must be
            // the highest priority task wanting to access the queue.
            if (queue->messagesWaiting > (unsigned PortBaseType)0)
            {
                // Remember our read position in case we are just peeking.
                originalReadPositon = queue->readFrom;

                // データ取り出し
                CopyDataFromQueue(queue, buffer);

                // 取り出し後っ読み取り位置を変える場合
                if (justPeeking == PD_FALSE)
                {
                    TraceQueueReceive(queue);

                    // We are actually removing data.
                    --(queue->messagesWaiting);

                    #if (CONFIG_USE_MUTEXES == 1)
                    {
                        if (queue->isMutex == QUEUE_QUEUE_IS_MUTEX)
                        {
                            // Record the information required to implement
                            // priority inheritance should it become necessary.
                            queue->mutexHolder = TaskGetCurrentTaskHandle();
                        }
                    }
                    #endif

                    if (ListListIsEmpty(&(queue->tasksWaitingToSend)) == PD_FALSE)
                    {
                        if (TaskRemoveFromEventList(&(queue->tasksWaitingToSend)) == PD_TRUE)
                        {
                            PortYieldWithinAPI();
                        }
                    }
                }
                // データ取り出し後, 読み取り位置を変えない場合
                else
                {
                    TraceQueuePeek(queue);

                    // The data is not being removed, so reset the read pointer.
                    queue->readFrom = originalReadPositon;

                    // The data is being left in the queue, so see if there are
                    // any other taasks waiting for the data.
                    if (ListListIsEmpty(&(queue->tasksWaitingToReceive)) == PD_FALSE)
                    {
                        // Tasks that are removed from the event list will get added to
                        // the pending ready list as the scheduler is still suspended.
                        if (TaskRemoveFromEventList(&(queue->tasksWaitingToReceive)) != PD_FALSE)
                        {
                            // The task waiting has a higher priority than this task.
                            PortYieldWithinAPI();
                        }
                    }
                }

                TaskExitCritical();
                return PD_PASS;
            }

            // Queue内にデータが存在しないとき.
            else
            {
                // 待ち時間が0のとき. 
                // この時は, 待機せずこの関数から抜ける.
                if (ticksToWait == (PortTickType)0)
                {
                    // The queue was empty and no block time is specified (or
                    // the block time has expired) so leave now.
                    TaskExitCritical();
                    TraceQueueReceiveFailed(queue);
                    return ERR_QUEUE_EMPTY;
                }
                // 待機時間が残っているときで, タイムアウトの設定がされていないとき
                else if (entryTimeSet == PD_FALSE)
                {
                    // The queue was empty and a block time was specified so
                    // configure the timeout structure.
                    TaskSetTimeOutState(&timeOut);
                    entryTimeSet = PD_TRUE;
                }
            }
        }
        TaskExitCritical();

        // Interrupts and other tasks can send to and receive from the queue
        // now the critical section has been exited.

        TaskSuspendAll();
        LockQueue(queue);

        // タイムアウトしていないか? 
        // タイムアウトしていないとき
        // Update the timeout state to see if it has expired yet.
        if (TaskCheckForTimeOut(&timeOut, &ticksToWait) == PD_FALSE)
        {
            // Queue内にデータがないとき
            if (IsQueueEmpty(queue) != PD_FALSE)
            {
                TraceBlockingOnQueueReceive(queue);

                #if (CONFIG_USE_MUTEXES == 1)
                {
                    if (queue->isMutex == QUEUE_QUEUE_IS_MUTEX)
                    {
                        PortEnterCritical();
                        {
                            TaskPriorityInherit((void *)queue->mutexHolder);
                        }
                        PortExitCritical();
                    }
                }
                #endif

                TaskPlaceOnEventList(&(queue->tasksWaitingToReceive), ticksToWait);
                UnlockQueue(queue);
                if (TaskResumeAll() == PD_FALSE)
                {
                    PortYieldWithinAPI();
                }
            }

            // Queue内にデータがないとき
            else
            {
                // Try again.
                UnlockQueue(queue);
                (void)TaskResumeAll();
            }
        }

        // タイムアウトしているとき
        else
        {
            UnlockQueue(queue);
            (void)TaskResumeAll();
            TraceQueueReceiveFailed(queue);
            return ERR_QUEUE_EMPTY;
        }
    }
}

signed PortBaseType QueueReceiveFromISR(QueueHandle queueFrom, void * const buffer, signed PortBaseType *higherPriorityTaskWoken)
{
    signed PortBaseType ret;

    Queue *queue;

    queue = (Queue *)queueFrom;

    // We cannot block from an ISR, so check there is data available.
    if (queue->messagesWaiting > (unsigned PortBaseType)0)
    {
        TraceQueueReceiveFromISR(queue);

        CopyDataFromQueue(queue, buffer);
        --(queue->messagesWaiting);

        /* If the queue is locked we will not modify the event list.  Instead
        we update the lock count so the task that unlocks the queue will know
        that an ISR has removed data while the queue was locked. */
        if (queue->rxLock == QUEUE_UNLOCKED)
        {
            if (ListListIsEmpty(&(queue->tasksWaitingToSend)) == PD_FALSE)
            {
                if (TaskRemoveFromEventList(&(queue->tasksWaitingToSend)) != PD_FALSE)
                {
                    // The task waiting has a higher priority then us so force a context switch.
                    if (higherPriorityTaskWoken != NULL)
                    {
                        *higherPriorityTaskWoken = PD_TRUE;
                    }
                }
            }
        }
        else
        {
            // Increment the lock count so the task that queue knows that data was removed while it was locked.
            ++(queue->rxLock);
        }

        ret = PD_PASS;
    }
    else
    {
        ret = PD_FAIL;
        TraceQueueReceiveFromISRFailed(queue);
    }

    return ret;
}

unsigned PortBaseType QueueMessagesWaiting(const QueueHandle queue)
{
    unsigned PortBaseType ret;

    TaskEnterCritical();
    {
        ret = ((Queue *)queue)->messagesWaiting;
    }
    TaskExitCritical();

    return ret;
}

unsigned PortBaseType QueueMessagesWaitingFromISR(const QueueHandle queue)
{
    unsigned PortBaseType ret;

    ret = ((Queue *)queue)->messagesWaiting;

    return ret;
}

void QueueDelete(QueueHandle queueToDelete)
{
    Queue *queue;

    queue = (Queue *)queueToDelete;

    TraceQueueDelete(queue);

    PortFree(queue->head);
    PortFree(queue);
}

static void CopyDataToQueue(Queue *queue, const void *itemToQueue, PortBaseType position) 
{
    if (queue->itemSize == (unsigned PortBaseType) 0)
    {
        #if(CONFIG_USE_MUTEXES == 1)
        {
            if (queue->isMutex == QUEUE_QUEUE_IS_MUTEX)
            {
                // The mutex is no longer being held.
                TaskPriorityDisinherit((void *)queue->mutexHolder);
                queue->mutexHolder = NULL;
            }
        }
        #endif
    }
    else if (position == QUEUE_SEND_TO_BACK)
    {
        memcpy((void *)queue->writeTo, itemToQueue, (unsigned)queue->itemSize);
        queue->writeTo += queue->itemSize;
        if (queue->writeTo >= queue->tail)
        {
            queue->writeTo = queue->head;
        }

    }
    else
    {
        memcpy((void *)queue->readFrom, itemToQueue, (unsigned)queue->itemSize);
        queue->readFrom -= queue->itemSize;
        if (queue->readFrom < queue->head)
        {
            queue->readFrom = (queue->tail - queue->itemSize);
        }
    }

    ++(queue->messagesWaiting);
}

static void CopyDataFromQueue(Queue * const queue, const void *buffer)
{
    if (queue->isMutex != QUEUE_QUEUE_IS_MUTEX)
    {
        queue->readFrom += queue->itemSize;
        if (queue->readFrom >= queue->tail)
        {
            queue->readFrom = queue->head;
        }
        memcpy((void *)buffer, (void *)queue->readFrom, (unsigned)queue->itemSize);
    }
}


static void UnlockQueue(Queue *queue)
{
    // THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED.

    // The lock counts contains the number of extra data items placed or
    // removed from the queue while the queue was locked.  When a queue is
    // locked items can be added or removed, but the event lists cannot be
    // updated.

    TaskEnterCritical();
    {
        // See if data added to the queue while it was locked.
        while (queue->txLock > QUEUE_LOCKED_UNMODIFIED)
        {
            // Data was posted while the queue was locked.  Are any tasks
            // blocked waiting for data to become available?

            // Tasks that are removed from the event list will get added to
            // the pending ready list as the scheduler is still suspended.
            if (ListListIsEmpty(&(queue->tasksWaitingToReceive)) == PD_FALSE)
            {
                if (TaskRemoveFromEventList(&(queue->tasksWaitingToReceive)) != PD_FALSE)
                {
                    // The task waiting has a higher priority so reord taht a
                    // context switch is required.
                    TaskMissedYield();
                }
            }
            else
            {
                break;
            }

            --(queue->txLock);
        }
        queue->txLock = QUEUE_UNLOCKED;
    }
    TaskExitCritical();

    // Do the same for the Rx lock.
}

static signed PortBaseType IsQueueEmpty(const Queue *queue)
{
    signed PortBaseType ret;

    TaskEnterCritical();
    {
        if (queue->messagesWaiting == 0)
        {
            ret = PD_TRUE;
        }
        else
        {
            ret = PD_FALSE;
        }
    }
    TaskExitCritical();

    return ret;
}

signed PortBaseType QueueIsQueueEmptyFromISR(const QueueHandle queue)
{
    signed PortBaseType ret;

    if (((Queue *)queue)->messagesWaiting == 0)
    {
        ret = PD_TRUE;
    }
    else
    {
        ret = PD_FALSE;
    }

    return ret;
}

static signed PortBaseType IsQueueFull(const Queue *queue)
{
    signed PortBaseType ret;

    TaskEnterCritical();
    {
        if (queue->messagesWaiting == queue->length)
        {
            ret = PD_TRUE;
        }
        else
        {
            ret = PD_FALSE;
        }
    }
    TaskExitCritical();

    return ret;
}

signed PortBaseType QueueIsQueueFullFromISR(const QueueHandle queue)
{
    signed PortBaseType ret;
    
    if (((Queue *)queue)->messagesWaiting == ((Queue *)queue)->length)
    {
        ret = PD_TRUE;
    }
    else
    {
        ret = PD_FALSE;
    }

    return ret;
}