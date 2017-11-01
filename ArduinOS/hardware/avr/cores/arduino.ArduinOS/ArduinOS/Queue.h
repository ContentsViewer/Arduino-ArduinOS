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


#ifndef ARDUINOS_QUEUE_H
#define ARDUINOS_QUEUE_H

#ifndef ARDUINOS_H
    #error "include ArduinOS.h" must appear in source files before "include Queue.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

    //
    // Type by which queues are referenced.  For example, a call to QueueCreate()
    // returns an QueueHandle variable that can then be used as a parameter to
    // QueueSend(), QueueReceive(), etc.
    //
    typedef void * QueueHandle;

#define QUEUE_SEND_TO_BACK (0)
#define QUEUE_SEND_TO_FRONT (1)

    // For interbal use only. These definitions must those in Queue.cpp
#define QUEUE_QUEUE_TYPE_BASE (0U)
#define QUEUE_QUEUE_TYPE_SET (0U)
#define QUEUE_QUEUE_TYPE_MUTEX (1U)
#define QUEUE_QUEUE_TYPE_COUNTING_SEMAPHORE (2U)
#define QUEUE_QUEUE_TYPE_BINARY_SEMAPHORE (3U)
#define QUEUE_QUEUE_TYPE_RECURSIVE_MUTEX (4U)


/*
// Creates a new queue instance.This allocates the storage required by the
// new queue and returns a handle for the queue.
//
// @param queueLength:
//  The maximum number of items that the queue can contain.
//
// @param itemSize:
//  The number of bytes each item in the queue will require.
//  Items are queued by copy, not by reference, so this is the number of bytes
//  that will be copied for each posted item.Each item on the queue must be
//  the same size.
//
// @return:
//  If the queue is successfully create then a handle to the newly
//  created queue is returned.If the queue cannot be created then 0 is
//  returned.
//
// Example usage :

struct Message
{
    char messageID;
    char data[20];
};

void Task(void *parameters)
{
    QueueHandle queue1, queue2;

    // Create a queue capable of containing 10 unsigned long values.
    queue1 = QueueCreate(10, sizeof(unsigned long));
    if (queue == 0)
    {
        // Queue was not created and must not be used.
    }

    // Create a queue capable of containing 10 pointers to Message structures.
    // These shoukd be passed by pointer as the contain a lot of data.
    queue2 = QueueCreate(10, sizeof(struct Message *));
    if (queue2 == 0)
    {
        // Queue was not created and must not be used.
    }

    // ... Rest of task code.
}

*/
#define QueueCreate(queueLength, itemSize) QueueGenericCreate(queueLength, itemSize, QUEUE_QUEUE_TYPE_BASE)

/*
// Queue先頭にアイテムを置きます. キューに置かれるアイテムは参照ではなくコピーです.
// この関数は割り込み関数の中で呼ばないようにしてください.
// 割り込み内で追加を行いたい場合はQueueSendFromISR()を使用してください.
//
// Queueの容量がなくアイテムが入れられない場合, ticksToWaitの間待機します.
// 待機後, もう一度アイテムを入れようとしますがそれでも入れられない場合はERR_QUEUE_FULLを返します.
//
// This is a macro that calls QueueGenericSend().
//
// Post an item to the front of a queue.The item is queued by copy, not by
// reference.This function must not be called from an interrupt service
// routine.See QueueSendFromISR() for an alternative which may be used
// in an ISR.
//
// @param queue:
//  The handle to the queue on which the item is to be posted.
//
// @param itemToQueue:
//  A pointer to the item that is to be placed on the
//  queue.The size of the items the queue will hold was defined when the
//  queue was created, so this many bytes will be copied from itemToQueue
//  into the queue storage area.
//
// @param ticksToWait:
//  The maximum amount of time the task should block
//  waiting for space to become available on the queue, should it already
//  be full.The call will return immediately if this is set to 0 and the
//  queue is full.The time is defined in tick periods so the constant
//  PORT_TICK_RATE_MS should be used to convert to real time if this is required.
//
// @return pdTRUE:
//  if the item was successfully posted, otherwise ERR_QUEUE_FULL.
//
// Example usage :


struct Message
{
    char messageID;
    char data[20];
};

struct Message message;

unsigned var = 10UL;

void Task(void *parameters)
{
    QueueHandle queue1, queue2;
    struct Message *messagePointer;

    // Create a queue capable of containing 10 unsigned long values.
    queue1 = QueueCreate(10, sizeof(unsigned long));

    // Create a queue capable of containing 10 pointers to Message structures.
    // These should be passed by pointer as they contain a lot of data.
    queue2 = QueueCreate(10, sizeof(struct Message *));

    // ...

    if (queue1 != 0)
    {
        // Send an unsigned long. Wait for 10 ticks for space to become
        // available if necessary.
        if (QueueSendToFront(queue1, (void *)&var, (PortTickType)10) != PD_PASS)
        {
            // Failed to post the message, even after 10 ticks.
        }
    }

    if (queue2 != 0)
    {
        // Send a pointer to struct Message object. Don't block if
        // queue is already full.
        messagePointer = &message;
        QueueSendToFront(queue2, (void *)&messagePointer, (PortTickType)0);
    }

    // ... Rest of task code.
}
*/
#define QueueSendToFront(queue, itemToQueue, ticksToWait) \
    QueueGenericSend((queue), (itemToQueue), (ticksToWait), QUEUE_SEND_TO_FRONT)

/*
// This is a macro that calls QueueGenericSend().
//
// Post an item to the back of a queue.The item is queued by copy, not by
// reference.This function must not be called from an interrupt service
// routine.See QueueSendFromISR() for an alternative which may be used
// in an ISR.
//
// @param queue:
//  The handle to the queue on which the item is to be posted.
//
// @param itemToQueue:
//  A pointer to the item that is to be placed on the
//  queue.The size of the items the queue will hold was defined when the
//  queue was created, so this many bytes will be copied from itemToQueue
//  into the queue storage area.
//
// @param ticksToWait:
//  The maximum amount of time the task should block
//  waiting for space to become available on the queue, should it already
//  be full.The call will return immediately if this is set to 0 and the queue
//  is full.The  time is defined in tick periods so the constant
//  PORT_TICK_RATE_MS should be used to convert to real time if this is required.
//
// @return:
//  PD_TRUE if the item was successfully posted, otherwise ERR_QUEUE_FULL.
//
// Example usage :

struct Message
{
    char messageID;
    char data[20];
};
Message message;

unsigned long var = 10UL;

void Task(void *parameters)
{
    QueueHandle queue1, queue2;
    struct Message *messagePointer;

    // Create a queue capable of containing 10 unsigned long values.
    queue1 = QueueCreate(10, sizeof(unsigned long));

    // Create a queue capable of containing 10 pointers to Message structures.
    // These should be passed by pointer as they contain a lot of data.
    queue2 = QueueCreate(10, sizeof(Message *));

    // ...

    if (queue1 != 0)
    {
        // Send an unsigned long. Wait fot 10 ticks for space to become available
        // if necessary.
        if (QueueSendToBack(queue1, (void *)&var, (PortTickType)10) != PD_PASS)
        {
            // Failed to post the message,  even after 10 ticks.
        }
    }

    if (queue2 != 0)
    {
        // Send a pointer to a struct a Message object. Don't block if the
        // queue is already full.
        messagePointer = &message;
        QueueSendToBack(queue2, (void *)&messagePointer, (PortTickType)0);

    }

    // ...Rest of task code.
}
*/
#define QueueSendToBack(queue, itemToQueue, ticksToWait) \
    QueueGenericSend((queue), (itemToQueue), (ticksToWait), QUEUE_SEND_TO_BACK)


/**
// This is a macro that calls QueueGenericSend(). It is
// equivalent to QueueSendToBack().
//
// Post an item on a queue.  The item is queued by copy, not by reference.
// This function must not be called from an interrupt service routine.
// See QueueSendFromISR () for an alternative which may be used in an ISR.
//
// @param queue:
//  The handle to the queue on which the item is to be posted.
//
// @param itemToQueue:
//  A pointer to the item that is to be placed on the
//  queue.  The size of the items the queue will hold was defined when the
//  queue was created, so this many bytes will be copied from pvItemToQueue
//  into the queue storage area.
//
// @param ticksToWait:
//  The maximum amount of time the task should block
//  waiting for space to become available on the queue, should it already
//  be full.  The call will return immediately if this is set to 0 and the
//  queue is full.  The time is defined in tick periods so the constant
//  PORT_TICK_RATE_MS should be used to convert to real time if this is required.
//
// @return PD_TRUE:
//  if the item was successfully posted, otherwise ERR_QUEUE_FULL.
//
// Example usage:

struct Message
{
char messageID;
char data[ 20 ];
};

Message message;

unsigned long var = 10UL;

void Task( void *parameters )
{
    QueueHandle queue1, queue2;
    Message *messagePointer;

    // Create a queue capable of containing 10 unsigned long values.
    queue1 = QueueCreate( 10, sizeof( unsigned long ) );

    // Create a queue capable of containing 10 pointers to AMessage structures.
    // These should be passed by pointer as they contain a lot of data.
    queue2 = QueueCreate( 10, sizeof( struct Message * ) );

    // ...

    if( queue1 != 0 )
    {
        // Send an unsigned long.  Wait for 10 ticks for space to become
        // available if necessary.
        if( QueueSend( queue1, ( void * ) &var, ( PortTickType ) 10 ) != PD_PASS )
        {
            // Failed to post the message, even after 10 ticks.
        }
    }

    if( xQueue2 != 0 )
    {
        // Send a pointer to a struct AMessage object.  Don't block if the
        // queue is already full.
        messagePointer = &message;
        QueueSend( queue2, ( void * ) &messagePointer, ( PortTickType ) 0 );
    }

    // ... Rest of task code.
}
*/
#define QueueSend( queue, itemToQueue, ticksToWait ) \
    QueueGenericSend( ( queue ), ( itemToQueue ), ( ticksToWait ), QUEUE_SEND_TO_BACK )

/*
// It is preferred that the macros QueueSend(), QueueSendToFront() and
// QueueSendToBack() are used in place of calling this function directly.
//
// Post an item on a queue.The item is queued by copy, not by reference.
// This function must not be called from an interrupt service routine.
// See QueueSendFromISR() for an alternative which may be used in an ISR.
//
// @param queue:
//  The handle to the queue on which the item is to be posted.
//
// @param itemToQueue:
//  A pointer to the item that is to be placed on the
//  queue.The size of the items the queue will hold was defined when the
//  queue was created, so this many bytes will be copied from itemToQueue
//  into the queue storage area.
//
// @param ticksToWait:
//  The maximum amount of time the task should block
//  waiting for space to become available on the queue, should it already
//  be full.The call will return immediately if this is set to 0 and the
//  queue is full.The time is defined in tick periods so the constant
//  PORT_TICK_RATE_MS should be used to convert to real time if this is required.
//
// @param copyPosition:
//  Can take the value QUEUE_SEND_TO_BACK to place the
//  item at the back of the queue, or QUEUE_SEND_TO_FRONT to place the item
//  at the front of the queue(for high priority messages).
//
// @return:
//  PD_TRUE if the item was successfully posted, otherwise ERR_QUEUE_FULL.
//
// Example usage :

struct Message
{
    char messageID;
    char data[20];
};

Message message;

unsigned long var = 10UL;

void Task(void *parameters)
{
    QueueHandle queue1, queue2;
    Message *messagePointer;

    // Create a queue capable of containing 10 unsigned long values.
    queue1 = QueueCreate(10, sizeof(unsigned long));

    // Create a queue capable of containing 10 pointers to AMessage structures.
    // These should be passed by pointer as they contain a lot of data.
    queue2 = QueueCreate(10, sizeof(struct Message *));

    // ...

    if (queue1 != 0)
    {
        // Send an unsigned long.  Wait for 10 ticks for space to become
        // available if necessary.
        if (QueueGenericSend(queue1, (void *)&var, (PortTickType)10, QUEUE_SEND_TO_BACK) != PD_PASS)
        {
            // Failed to post the message, even after 10 ticks.
        }
    }

    if (xQueue2 != 0)
    {
        // Send a pointer to a struct AMessage object.  Don't block if the
        // queue is already full.
        messagePointer = &message;
        QueueSend(queue2, (void *)&messagePointer, (PortTickType)0), QUEUE_SEND_TO_BACK;
    }

    // ... Rest of task code.
}
*/
    signed PortBaseType QueueGenericSend(QueueHandle queue, const void * const itemToQueue, PortTickType ticksToWait, PortBaseType copyPosition);

    /*
    //
    // This is a macro that calls the QueueGenericReceive() function.
    //
    // Receive an item from a queue without removing the item from the queue.
    // The item is received by copy so a buffer of adequate size must be
    // provided.The number of bytes copied into the buffer was defined when
    // the queue was created.
    //
    // Successfully received items remain on the queue so will be returned again
    // by the next call, or a call to QueueReceive().
    //
    // This macro must not be used in an interrupt service routine.
    //
    // @param queue:
    //  The handle to the queue from which the item is to be received.
    //
    // @param buffer:
    //  Pointer to the buffer into which the received item will be copied.
    //
    // @param ticksToWait:
    //  The maximum amount of time the task should block
    //  waiting for an item to receive should the queue be empty at the time
    //  of the call.The time is defined in tick periods so the constant
    //  PORT_TIC_RATE_MS should be used to convert to real time if this is required.
    //  QueuePeek() will return immediately if ticksToWait is 0 and the queue
    //  is empty.
    //
    // @return:
    //  pdTRUE if an item was successfully received from the queue,
    //  otherwise pdFALSE.
    //
    // Example usage :

    struct Message
    {
        char messageID;
        char data[20];
    };

    Message message;

    QueueHandle queue;

    // Task to create a queue and post a value.
    void Task(void *parameters)
    {
        Message *messagePointer;

        // Create a queue capable of containing 10 pointers to AMessage structures.
        // These should be passed by pointer as they contain a lot of data.
        queue = QueueCreate(10, sizeof(struct Message *));

        if (queue == 0)
        {
            // Failed to create the queue.
        }
        // Send a pointer to struct Message object. Don't block if the queue is already full.
        messagePointer = &message;
        QueueSend(queue, (void *)&messagePointer, (PortTickType)0);

        // ... Rest of task code.
    }

    // Task to peek the data from the queue.
    void DifferentTask(void *parameters)
    {
        struct Message *rxedMessage;

        if (queue != 0)
        {
            // Peek a message on the created queue. Block for 10 ticks if a
            // message is not immediately available.
            if (QueuePeek(queue, &(rxedMessage), (PortTickType)10))
            {
                // rxedMessage now points to the struct Message variable posted
                // by Task, but the item still remains on the queue.
            }
        }

        // ...Rest of task code.
    }
    */
#define QueuePeek(queue, buffer, ticksToWait) QueueGenericReceive((queue), (buffer), (ticksToWait), PD_TRUE)

    /*
    // This is a macro that calls the QueueGenericReceive() function.
    //
    // Receive an item from a queue.The item is received by copy so a buffer of
    // adequate size must be provided.The number of bytes copied into the buffer
    // was defined when the queue was created.
    //
    // Successfully received items are removed from the queue.
    //
    // This function must not be used in an interrupt service routine.See
    // QueueReceiveFromISR for an alternative that can.
    //
    // @param queue:
    //  The handle to the queue from which the item is to be received.
    //
    // @param buffer:
    //  Pointer to the buffer into which the received item will be copied.
    //
    // @param ticksToWait:
    //  The maximum amount of time the task should block
    //  waiting for an item to receive should the queue be empty at the time
    //  of the call.QueueReceive() will return immediately if ticksToWait
    //  is zero and the queue is empty.The time is defined in tick periods so the
    //  constant PORT_TICK_RATE_MS should be used to convert to real time if this is
    //  required.
    //
    // @return:
    //  PD_TRUE if an item was successfully received from the queue,
    //  otherwise PD_FALSE.
    //
    // Example usage :

    struct Message
    {
        char messageID;
        char data[20];
    };

    Message message;

    QueueHandle queue;

    // Task to create a queue and post a value.
    void Task(void *parameters)
    {
        Message *messagePointer;

        // Create a queue capable of containing 10 pointers to AMessage structures.
        // These should be passed by pointer as they contain a lot of data.
        queue = QueueCreate(10, sizeof(struct Message *));

        if (queue == 0)
        {
            // Failed to create the queue.
        }

        // ...

        // Send a pointer to struct Message object. Don't block if the queue is already full.
        messagePointer = &message;
        QueueSend(queue, (void *)&messagePointer, (PortTickType)0);

        // ... Rest of task code.
    }

    // Task to peek the data from the queue.
    void DifferentTask(void *parameters)
    {
        struct Message *rxedMessage;

        if (queue != 0)
        {
            // Receive a message on the created queue. Block for 10 ticks if a
            // message is not immediately available.
            if (QueuePeek(queue, &(rxedMessage), (PortTickType)10))
            {
                // rxedMessage now points to the struct Message variable posted
                // by Task.
            }
        }

        // ...Rest of task code.
    }

    */
#define QueueReceive(queue, buffer, ticksToWait) QueueGenericReceive((queue), (buffer), (ticksToWait), PD_FALSE)

    /*
    // It is preferred that the macro QueueReceive() be used rather than calling
    // this function directly.
    //
    // Receive an item from a queue.The item is received by copy so a buffer of
    // adequate size must be provided.The number of bytes copied into the buffer
    // was defined when the queue was created.
    //
    // This function must not be used in an interrupt service routine.See
    // QueueReceiveFromISR for an alternative that can.
    //
    // @param queue:
    //  The handle to the queue from which the item is to be received.
    //
    // @param buffer:
    //  Pointer to the buffer into which the received item will be copied.
    //
    // @param ticksToWait:
    //  The maximum amount of time the task should block
    //  waiting for an item to receive should the queue be empty at the time
    //  of the call.The time is defined in tick periods so the constant
    //  PORT_TICK_RATE_MS should be used to convert to real time if this is required.
    //  QueueGenericReceive() will return immediately if the queue is empty and
    //  ticksToWait is 0.
    //
    // @param justPeek:
    //  When set to true, the item received from the queue is not
    //  actually removed from the queue - meaning a subsequent call to
    //  QueueReceive() will return the same item.When set to false, the item
    //  being received from the queue is also removed from the queue.
    //
    // @return:
    //  PD_TRUE if an item was successfully received from the queue,
    //  otherwise PD_FALSE.
    //
    // Example usage :

    struct Message
    {
        char messageID;
        char data[20];
    };

    Message message;

    QueueHandle queue;

    // Task to create a queue and post a value.
    void Task(void *parameters)
    {
        Message *messagePointer;

        // Create a queue capable of containing 10 pointers to AMessage structures.
        // These should be passed by pointer as they contain a lot of data.
        queue = QueueCreate(10, sizeof(struct Message *));

        if (queue == 0)
        {
            // Failed to create the queue.
        }

        // ...

        // Send a pointer to struct Message object. Don't block if the queue is already full.
        messagePointer = &message;
        QueueSend(queue, (void *)&messagePointer, (PortTickType)0);

        // ... Rest of task code.
    }

    // Task to peek the data from the queue.
    void DifferentTask(void *parameters)
    {
        struct Message *rxedMessage;

        if (queue != 0)
        {
            // Receive a message on the created queue. Block for 10 ticks if a
            // message is not immediately available.
            if (QueueGenericReceive(queue, &(rxedMessage), (PortTickType)10, PD_FALSE))
            {
                // rxedMessage now points to the struct Message variable posted
                // by Task.
            }
        }

        // ...Rest of task code.
    }

    */
    signed PortBaseType QueueGenericReceive(QueueHandle queue, void * const buffer, PortTickType ticksToWait, PortBaseType justPeek);

    /*
    // Return the number of message stored ina queue.
    //
    // @param queue:
    //  A handle to the queue being queried.
    //
    // @return:
    //  The number of message available in the queue.
    */
    unsigned PortBaseType QueueMessagesWaiting(const QueueHandle queue);

    /*
    // Delete a queue - freeing all the memory allocated for storing of items
    // placed on the queue.
    //
    // @param queue:
    //  A handle to the queue to be deleted.
    */
    void QueueDelete(QueueHandle queue);


    /*
    //
    // This is a macro that calls QueueGenericSendFromISR().
    //
    // Post an item to the front of a queue.  It is safe to use this macro from
    // within an interrupt service routine.
    //
    // Items are queued by copy not reference so it is preferable to only
    // queue small items, especially when called from an ISR.  In most cases
    // it would be preferable to store a pointer to the item being queued.
    //
    // @param queue:
    //  The handle to the queue on which the item is to be posted.
    //
    // @param itemToQueue:
    //  A pointer to the item that is to be placed on the
    //  queue.  The size of the items the queue will hold was defined when the
    //  queue was created, so this many bytes will be copied from itemToQueue
    //  into the queue storage area.
    //
    // @param higherPriorityTaskWoken:
    //  QueueSendToFrontFromISR() will set
    //  higherPriorityTaskWoken to pdTRUE if sending to the queue caused a task
    //  to unblock, and the unblocked task has a priority higher than the currently
    //  running task.  If QueueSendToFromFromISR() sets this value to pdTRUE then
    //  a context switch should be requested before the interrupt is exited.
    //
    // @return:
    //  pdTRUE if the data was successfully sent to the queue, otherwise
    //  ERR_QUEUE_FULL.
    //
    // Example usage for buffered IO (where the ISR can obtain more than one value
    // per call):

    void BufferISR(void)
    {
        char in;
        PortBaseType higherPriorityTaskWoken;

        // We have not woken a task at the start of the ISR.
        higherPriorityTaskWoken = PD_FALSE;

        // Loop until the buffer is empty.
        do
        {
            // Obtain a byte from the buffer.
            in = PortInputByte(RX_REGISTER_ADDRESS);

            // Port the byte.
            QueueSendToFrontFromISR(rxQueue, &in, &higherPriorityTaskWoken);
        } while (PortInputByte(BUFFER_COUNT));

        // Now the buffer is empty we can switch context if necessary.
        if (higherPriorityTaskWoken)
        {
            TaskYield();
        }
    }
    */
#define QueueSendToFrontFromISR(queue, itemToQueue, higherPriorityTaskWoken) \
    QueueGenericSendFromISR((queue), (itemToQueue), (higherPriorityTaskWoken), QUEUE_SEND_TO_FRONT)


    /*
    // This is a macro that calls QueueGenericSendFromISR().
    //
    // Post an item to the back of a queue.  It is safe to use this macro from
    // within an interrupt service routine.
    //
    // Items are queued by copy not reference so it is preferable to only
    // queue small items, especially when called from an ISR.  In most cases
    // it would be preferable to store a pointer to the item being queued.
    //
    // @param queue:
    //  The handle to the queue on which the item is to be posted.
    //
    // @param itemToQueue:
    //  A pointer to the item that is to be placed on the
    //  queue.  The size of the items the queue will hold was defined when the
    //  queue was created, so this many bytes will be copied from itemToQueue
    //  into the queue storage area.
    //
    // @param higherPriorityTaskWoken:
    //  QueueSendToBackFromISR() will set higherPriorityTaskWoken to PD_TRUE if
    //  sending to the queue caused a task to unblock, and the unblocked task
    //  has a priority higher than the currently running task.
    //  If QueueSendToBackFromISR() sets this value to pdTRUE then
    //  a context switch should be requested before the interrupt is exited.
    //
    // @return:
    //  PD_TRUE if the data was successfully sent to the queue, otherwise
    //  ERR_QUEUE_FULL.
    //
    // Example usage for buffered IO (where the ISR can obtain more than one value
    // per call):

    void BufferISR()
    {
        char input;
        PortBaseType higherPriorityTaskWoken;

        // We have not woken a task at the start of the ISR.
        higherPriorityTaskWoken = PD_FALSE;

        // Loop unitil the buffer is empty.
        do
        {
            // Obtain a byte from the buffer.
            input = PortInputByte(RX_REGISTER_ADDRESS);

            // Post the byte.
            QueueSendToBaackFromISR(rxQueue, &input, &higherPriorityTaskWoken);

        } while (PortInputByte(BUFFER_COUNT));

        // Now the buffer is empty we can switch context if necessary.
        if (higherPriorityTaskWoken)
        {
            TaskYield();
        }
    }

    */
#define QueueSendToBackFromISR(queue, itemToQueue, higherPriorityTaskWoken) \
    QueueGenericSendFromISR((queue), (itemToQueue), (higherPriorityTaskWoken), QUEUE_SEND_TO_BACK)

    /*
    //
    // This is a macro that calls QueueGenericSendFromISR().
    //
    // Post an item to the back of a queue.It is safe to use this function from
    // within an interrupt service routine.
    //
    // Items are queued by copy not reference so it is preferable to only
    // queue small items, especially when called from an ISR.In most cases
    // it would be preferable to store a pointer to the item being queued.
    //
    // @param queue:
    //  The handle to the queue on which the item is to be posted.
    //
    // @param itemToQueue:
    //  A pointer to the item that is to be placed on the
    //  queue.The size of the items the queue will hold was defined when the
    //  queue was created, so this many bytes will be copied from itemToQueue
    //  into the queue storage area.
    //
    // @param higherPriorityTaskWoken:
    //  QueueSendFromISR() will set
    //  higherPriorityTaskWoken to PD_TRUE if sending to the queue caused a task
    //  to unblock, and the unblocked task has a priority higher than the currently
    //  running task.If QueueSendFromISR() sets this value to PD_TRUE then
    //  a context switch should be requested before the interrupt is exited.
    //
    // @return:
    //  pdTRUE if the data was successfully sent to the queue, otherwise
    //  ERR_QUEUE_FULL.
    //
    // Example usage for buffered IO(where the ISR can obtain more than one value
    // per call) :

    void BufferISR()
    {
        char input;
        PortBaseType higherPriorityTaskWoken;

        // We have not woken a task at the start of the ISR.
        higherPriorityTaskWoken = PD_FALSE;

        // Loop unitil the buffer is empty.
        do
        {
            // Obtain a byte from the buffer.
            input = PortInputByte(RX_REGISTER_ADDRESS);

            // Post the byte.
            QueueSendFromISR(rxQueue, &input, &higherPriorityTaskWoken);

        } while (PortInputByte(BUFFER_COUNT));

        // Now the buffer is empty we can switch context if necessary.
        if (higherPriorityTaskWoken)
        {
            TaskYield();
        }
    }

    */
#define QueueSendFromISR(queue, itemToQueue, higherPriorityTaskWoken) \
    QueueGenericSendFromISR((queue), (itemToQueue), (higherPriorityTaskWoken), QUEUE_SEND_TO_BACK)

    /*
    //
    // It is preferred that the macros QueueSendFromISR(),
    // QueueSendToFrontFromISR() and QueueSendToBackFromISR() be used in place
    // of calling this function directly.
    //
    // Post an item on a queue.  It is safe to use this function from within an
    // interrupt service routine.
    //
    // Items are queued by copy not reference so it is preferable to only
    // queue small items, especially when called from an ISR.  In most cases
    // it would be preferable to store a pointer to the item being queued.
    //
    // @param queue:
    //  The handle to the queue on which the item is to be posted.
    //
    // @param itemToQueue:
    //  A pointer to the item that is to be placed on the
    //  queue.  The size of the items the queue will hold was defined when the
    //  queue was created, so this many bytes will be copied from itemToQueue
    //  into the queue storage area.
    //
    // @param higherPriorityTaskWoken:
    //  QueueGenericSendFromISR() will set
    //  higherPriorityTaskWoken to PD_TRUE if sending to the queue caused a task
    //  to unblock, and the unblocked task has a priority higher than the currently
    //  running task.  If QueueGenericSendFromISR() sets this value to PD_TRUE then
    //  a context switch should be requested before the interrupt is exited.
    //
    // @param copyPosition:
    //  Can take the value QUEUE_SEND_TO_BACK to place the
    //  item at the back of the queue, or QUEUE_SEND_TO_FRONT to place the item
    //  at the front of the queue (for high priority messages).
    //
    // @return:
    //  PD_TRUE if the data was successfully sent to the queue, otherwise
    //  ERR_QUEUE_FULL.
    //
    // Example usage for buffered IO (where the ISR can obtain more than one value
    // per call):

    void BufferISR()
    {
        char input;
        PortBaseType higherPriorityTaskWoken;

        // We have not woken a task at the start of the ISR.
        higherPriorityTaskWoken = PD_FALSE;

        // Loop unitil the buffer is empty.
        do
        {
            // Obtain a byte from the buffer.
            input = PortInputByte(RX_REGISTER_ADDRESS);

            // Post the byte.
            QueueGenericSendFromISR(rxQueue, &input, &higherPriorityTaskWoken, QUEUE_SEND_TO_BACK);

        } while (PortInputByte(BUFFER_COUNT));

        // Now the buffer is empty we can switch context if necessary.
        if (higherPriorityTaskWoken)
        {
            TaskYield();
        }
    }

    */
    signed PortBaseType QueueGenericSendFromISR(QueueHandle queue, const void * const itemToQueue, signed PortBaseType *higherPriorityTaskWoken, PortBaseType copyPosition);

    /*
    //
    // Receive an item from a queue.  It is safe to use this function from within an
    // interrupt service routine.
    //
    // @param queue:
    //  The handle to the queue from which the item is to be received.
    //
    // @param buffer:
    //  Pointer to the buffer into which the received item will be copied.
    //
    // @param taskWoken:
    //  A task may be blocked waiting for space to become
    //  available on the queue.  If QueueReceiveFromISR causes such a task to
    //  unblock taskWoken will get set to PD_TRUE, otherwise /taskWoken will
    //  remain unchanged.
    //
    // @return:
    //  PD_TRUE if an item was successfully received from the queue,
    //  otherwise PD_FALSE.
    //
    // Example usage:
    QueueHandle queue;

    // Function to create a queue and post some values.
    void Function(void *parameters)
    {
        char valueToPost;
        const PortTickType blockTime = (PortTickType)0xff;

        // Create a queue capable of containing 10 characters.
        queue = QueueCreate(10, sizof(char));
        if (queue == 0)
        {
            // Failed to create the queue.
        }

        // ...

        // Post some characters that will be used within an ISR. If the queue
        // is full then this task will block for BlockTime ticks.
        valueToPost = 'a';
        QueueSend(queue, (void *)&valueToPost, blockTime);
        valueToPost = 'b';
        QueueSend(queue, (void *)&valueToPost, blockTime);

        // ... Keep posting characters ... this task may block when the queue
        // becomes full.
        valueToPost = 'c';
        QueueSend(queue, (void *)&valueToPost, blockTime);
    }

    // ISR that outputs all the characters received on the queue.
    void ISRRoutine(void)
    {
        PortBaseType taskWokenByReceive = PD_FALSE;
        char rxedChar;

        while (QueueReceiveFromISR(queue, (void *)&rxedChar, &taskWokenByReceive))
        {
            // A character was received. Output the character now.
            OutputCharacter(rxedChar);

            // If removing the character from the queue woke the task that was
            // posting onto the queue taskWokenByReceive will have been set to
            // PD_TRUE.  No matter how many times this loop iterates only one
            // task will be woken.
        }

        if (taskWokenByReceive != (char)PD_FALSE)
        {
            TaskYield();
        }
    }

    */
    signed PortBaseType QueueReceiveFromISR(QueueHandle queue, void * const buffer, signed PortBaseType *higherPriorityTaskWoken);

    /*
    // Utilities to query queues that are safe to use from an ISR.  These utilities
    // should be used only from witin an ISR, or within a critical section.
    */
    signed PortBaseType QueueIsQueueEmptyFromISR(const QueueHandle queue);
    signed PortBaseType QueueisQueueFullFromISR(const QueueHandle queue);
    unsigned PortBaseType QueueMessagesWaitingFromISR(const QueueHandle queue);

    /*
    // For internal use only.  Use SemaphoreCreateMutex(),
    // or SemaphoreGetMutexHolder() instead of calling
    // these functions directly.
    */
    QueueHandle QueueCreateMutex(unsigned char queueType);


    /*
    // Reset a queue back to its original empty state.  PD_PASS is returned if the
    // queue is successfully reset.  PD_FAIL is returned if the queue could not be
    // reset because there are tasks blocked on the queue waiting to either
    // receive from the queue or send to the queue.
    */
#define QueueReset(queue) QueueGenericReset(queue, PD_FALSE);

    // Generic version of queue creation function, which is in turn called by
    // any queue, semaphore or mutex creation fuction or macro.
    QueueHandle QueueGenericCreate(unsigned PortBaseType queueLength, unsigned PortBaseType itemSize, unsigned char queueType);



#ifdef __cplusplus
}
#endif

#endif