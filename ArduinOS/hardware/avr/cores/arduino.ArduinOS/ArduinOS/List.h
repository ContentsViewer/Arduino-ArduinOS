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

#ifndef ARDUINOS_LIST_H
#define ARDUINOS_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

    struct ListItem
    {
        // リストされる変数, これが降順になるようにListItemを並べる.
        // The value being listed. In most cases this is used to sort the list in descending order.
        PortTickType itemValue;

        // Pointer to the next ListItem in the list.
        volatile struct ListItem *next;

        // Pointer to the previous ListItem in the list.
        volatile struct ListItem *previous;

        // このListItemを持つオブジェクト( 主にTCB )へのポインタ
        // Pointer to the object (normally a TCB) that contains the list item.
        // There is therefore a two way link between the object containing the list item and the list item itself.
        void *owner;

        // Pointer to the list in which this list item is placed (if any).
        void *container;
    };

    //
    // Memo:
    //  上でtypedefを使用できなかった.
    //  typedefで定義される前にListItemを使用したことが考えられる.
    //
    typedef struct ListItem ListItem;

    typedef struct 
    {
        PortTickType itemValue;
        volatile ListItem *next;
        volatile ListItem *previous;
    } MiniListItem;

    // Definition of the type of queu used by the scheduler.
    typedef struct 
    {
        volatile unsigned PortBaseType numberOfItems;

        // Used to walk through the list. 
        // Points to the last item returned by a call to ListGetOwnerOfNextEntry()
        volatile ListItem *index;

        // List item that contains the maximum possible item value meaning it is always at the end of the list and
        // is therefore used as a marker.
        volatile MiniListItem listEnd;
    } List;

    // Access macro to set the owner of a list item. The owner of a list item is
    // the object (usually a TCB) that contains the list item.
#define ListSetListItemOwner(listItem, ownerOfItem) \
    (listItem)->owner = (void *)(ownerOfItem)

// Access macro to get the owner of a list item. The owner of a list item is
// the object  (usually a TCB) that contains the list item.
#define ListGetListItemOwner(listItem) \
    (listItem)->owner

// Access macro to set the value of the list item.
// In most cases the value is used to sort the list in descending order.
#define ListSetListItemValue(listItem, value) \
    (listItem)->itemValue = (value)

// Access macro to retrieve the value of the list item.
// The value can represent anything - for example a the priority of a task,
// or the time at which a task should be unblocked.
#define ListGetListItemValue(listItem) \
    ((listItem)->itemValue)

// Access macro the retrieve the value of the list item at the head of a given list.
#define ListGetItemValueOfHeadEntry(list) \
    ((&((list)->listEnd))->next->itemValue)

// Access macro to determine if a list contains any items.
// The macro will only have the value true if the list is empty.
#define ListListIsEmpty(list) \
    ((list)->numberOfItems == (unsigned PortBaseType)0)

// Access macro to return the number of items in the list.
#define ListCurrentListLength(list) \
    ((list)->numberOfItems)

//
// Access function to obtain the owner of the next entry in a list.
//
// The list member index is used to walk through a list.
// Calling ListGetOwnerOfNextEntry increments index to the next item in the list
// and returns that entries owner parameeter. Using multiple calls to this function
// it is therfore possible to move through every item contained in a list.
//
// The owner parameter of a list item is a pointer to the object that owns the list item.
// In the scheduler this is normally a task control block.
// The owner parameter effectively creates a two way link between the list item and its owner.
//
// NOTE:
//  現在のindexのアイテムが対象とならないことに注意する.
//  index値をインクリメントしてからOwnerを取得する.
//
// @param list:
//  The list from which the next item owner is to be returned.
//
#define ListGetOwnerOfNextEntry(tcb, list)                                  \
{                                                                           \
    List* const constList = (list);                                         \
    /* Increment the index to the next item and return the item,*/          \
    /* ensuring we don't return the marker used at the end of the list.*/   \
    (constList)->index = (constList)->index->next;                          \
    if((constList)->index == (ListItem*)&((constList)->listEnd))            \
    {                                                                       \
        (constList)->index = (constList)->index->next;                      \
    }                                                                       \
    (tcb) = (constList)->index->owner;                                      \
}

//
// Access function to obtain the owner of the first entry in a list.
// Lists are normally sorted in ascending item value order.
//
// This function returns the owner member of the first item in the list.
// The owner parameter of a list item is a ponter to the object that owns
// the list item. In the scheduler this is normally a task control block.
// The owner parameter effectively creates a teo way link between the list
// item and its owner.
//
// @patam list: 
//  The list from which the owner of the head item is to be 
//  returned.
//
#define ListGetOwnerOfHeadEntry(list) \
    ((&((list)->listEnd))->next->owner)

//
// Check to see if a list item is within a list. The list item maintains a
// "container" pointer that points to the list it is in. All this macro does
// is check to see if the container and the list match.
//
// @param list: 
//  The list we want to know if the list item is within.
// @param listItem: 
//  The list item we want to know if is in the list.
// @return: 
//  PD_TRUE is the list item is in the list, otherwaise PD_FAILSE.
//  pointer against
//
#define ListIsContainedWithin(list, listItem) \
    ((listItem)->container == (void*)(list))

//
// Return the list a list item is contained within (referenced from).
//
// @param listItem: 
//  The list item being queried.
// @return: 
//  A pointer to the list object that taht references the listItem.
//
#define ListListItemContainer(listItem) \
    ((listItem)->container)

//
// This provides a crude means of knowing if a list has been initialised,
// as list->listEnd.itemValue is set to PORT_MAX_DELAY by the ListInitialise()
// function.
//
#define ListListIsInitialised(list) \
    ((list)->listEnd.itemValue == PORT_MAX_DELAY)

//
// Must be called before a list is used! This initialises all the members
// of the list structure and inserts the listEnd item into the list as a
// marker to the back of the list.
//
// @param list: 
//  Pointer to the lsit being initialised.
    void ListInitialise(List *list);

    //
    // Must be called before a list item is used. This sets the list container to
    // null so the item does not think that it is already contained in a list.
    //
    // @param item: 
    //  Pointer to the list item being initialised.
    //
    void ListInitialiseItem(ListItem *listItem);

    //
    // Insert a list item into a list.  The item will be inserted into the list in
    // a position determined by its item value (descending item value order).
    //
    // @param pxList:
    //  The list into which the item is to be inserted.
    //
    void ListInsert(List *list, ListItem *newListItem);

    //
    // Listの最後にアイテムを挿入する. 
    // この関数と似た挿入関数Insert()があるが, これは, itemValueが降順に並ぶように
    // 新規アイテムを挿入する. 一方この関数は, 単にリストの最後にアイテムを付け加えるだけである.
    // アイテム登録順にリストを並べたい時は, この関数を使うのがよいでしょう.
    // 
    // 仕組み:
    //  indexの後に挿入される. 挿入後, indexは挿入したアイテムのポインタをさす.
    //  要素読み込みの際では, ListGetOwnerOfNextEntry を用いるのだが,
    //  この関数は, 現在のindexの次のアイテムを対象とする. つまり, この関数がさすのは
    //  Listの先頭アイテムである.
    //
    // Insert a list item into a list.  The item will be inserted in a position
    // such that it will be the last item within the list returned by multiple
    // calls to ListGetOwnerOfNextEntry.
    //
    // The list member index is used to walk through a list.  Calling
    // ListGetOwnerOfNextEntry increments index to the next item in the list.
    // Placing an item in a list using ListInsertEnd effectively places the item
    // in the list position pointed to by index.  This means that every other
    // item within the list will be returned by ListGetOwnerOfNextEntry before
    // the index parameter again points to the item being inserted.
    //
    // @param list:
    //  The list into which the item is to be inserted.
    //
    // @param newListItem:
    //  The list item to be inserted into the list.
    //
    void ListInsertEnd(List *list, ListItem *newListItem);

    //
    // Remove an item from a list.  The list item has a pointer to the list that
    // it is in, so only the list item need be passed into the function.
    //
    // @param itemToRemove:
    //  The item to be removed.  The item will remove itself from
    //  the list pointed to by it's container parameter.
    //
    // @return 
    //  The number of items that remain in the list after the list item has
    //  been removed.
    //
    unsigned PortBaseType ListRemove(ListItem *itemToRemove);


#ifdef __cplusplus
}
#endif

#endif