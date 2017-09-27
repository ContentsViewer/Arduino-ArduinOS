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
#include "MegaOS.h"
#include "List.h"

//-----------------------------------------------------------
// PUBLIC LIST API
//-----------------------------------------------------------
void ListInitialise(List *list)
{
    // The list structure contains a list item which is used to mark the
    // end of the list. To initialise the list the last end is inserted as the only list entry.
    list->index = (ListItem *) &(list->listEnd);

    // listEndのitemValueはできる限り大きくする. これは, listの最後にあることを保証するためである.
    // The list end value is the highest possible value in the list 
    // to ensure it remains at the end of the list.
    list->listEnd.itemValue = PORT_MAX_DELAY;

    // The list end and next and previous pointers point to itself 
    // so we know when the list is empty.
    list->listEnd.next = (ListItem *) &(list->listEnd);
    list->listEnd.previous = (ListItem *) &(list->listEnd);

    list->numberOfItems = (unsigned PortBaseType) 0U;
}

void ListInitialiseItem(ListItem *item)
{
    // Make sure the list item is not recorded as being on a list.
    item->container = NULL;
}

void ListInsertEnd(List *list, ListItem *newListItem)
{
    volatile ListItem *index;

    // Insert a new item into list, but rather than sort the list,
    // makes the new list item the last item to be removed by a call
    // to ListGetOwnerOfNextEntry.
    // This means it has to be the item pointed to by the index member.
    index = list->index;

    // リンクつなぎ替え; つなぎ替え箇所4つ
    // index.next
    // inserted.next & previous
    // index + 1.previous
    newListItem->next = index->next;
    newListItem->previous = list->index;
    index->next->previous = (volatile ListItem *)newListItem;
    index->next = (volatile ListItem *)newListItem;
    list->index = (volatile ListItem *)newListItem;

    // Remember which list the item is in.
    newListItem->container = (void *)list;

    (list->numberOfItems)++;
}

void ListInsert(List *list, ListItem *newListItem)
{
    volatile ListItem *iterator;
    PortTickType valueOfInsertion;

    // Insert the new list item into the list, sorted in listItem order.
    valueOfInsertion = newListItem->itemValue;

    // List内に同じvalueのitemが存在しているときは, 新しいitem(挿入したいitem)をそのあとに置くべきである.
    // 
    // If the list already contains a list item with the same item value then
    // the new list item should be placed after it. This ensures that TCB's which
    // are stored in ready lists (all of which have same listItem value)
    // get an equal share of the CPU. However, if the itemValue is the same as
    // to guard against this by checking the value first and modifying the
    // algorithm slightly if necessary.
    if (valueOfInsertion == PORT_MAX_DELAY)
    {
        iterator = list->listEnd.previous;
    }
    else
    {
        // *** NOTE ************************************************************
        // If you find your application is crashing here then likely causes are:
        //      1) Stack overflow
        //      2) Incorrect interrupt priority assignment, especially on Cortex-M3
        //         parts where numerically high priority values denote low actual
        //         interrupt priories, which can seen counter intuitive.
        //      3) Calling an API function from within a critical section or when
        //         the scheduler is suspended.
        //      4) Using a queue or semaphore before it has been initialised or
        //         before the scheduler has been started (are interrupts firing
        //         before TaskStartScheduler() has been called?).
        // **********************************************************************
        
        // List先頭から挿入位置を探す. itemValueが降順(大きい順)になるように挿入される.
        for (iterator = (ListItem*)&(list->listEnd); iterator->next->itemValue <= valueOfInsertion; iterator = iterator->next)
        {
            // There is nothing to do here,  we are just iterating to the wanted insertion position.
        }
    }

    newListItem->next = iterator->next;
    newListItem->next->previous = (volatile ListItem*)newListItem;
    newListItem->previous = iterator;
    iterator->next = (volatile ListItem*)newListItem;

    // Remember which list the item is in.
    // This allows fast removal of the item later.
    newListItem->container = (void*)list;

    (list->numberOfItems)++;
}

unsigned PortBaseType ListRemove(ListItem *itemToRemove)
{
    List *list;

    itemToRemove->next->previous = itemToRemove->previous;
    itemToRemove->previous->next = itemToRemove->next;

    // The list item knows which list it is in.
    // Obtain the list from the list item.
    list = (List *)itemToRemove->container;

    // Make sure the index is left pointing to a valid item.
    if (list->index == itemToRemove)
    {
        list->index = itemToRemove->previous;
    }

    itemToRemove->container = NULL;
    (list->numberOfItems)--;

    return list->numberOfItems;
}

// --- End PUBLIC LIST API ----------------------------------------