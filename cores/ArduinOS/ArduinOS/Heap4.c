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
FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
All rights reserved

VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

This file is part of the FreeRTOS distribution.

FreeRTOS is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License (version 2) as published by the
Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

***************************************************************************
>>!   NOTE: The modification to the GPL is included to allow you to     !<<
>>!   distribute a combined work that includes FreeRTOS without being   !<<
>>!   obliged to provide the source code for proprietary components     !<<
>>!   outside of the FreeRTOS kernel.                                   !<<
***************************************************************************

FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  Full license text is available on the following
link: http://www.freertos.org/a00114.html

***************************************************************************
*                                                                       *
*    FreeRTOS provides completely free yet professionally developed,    *
*    robust, strictly quality controlled, supported, and cross          *
*    platform software that is more than just the market leader, it     *
*    is the industry's de facto standard.                               *
*                                                                       *
*    Help yourself get started quickly while simultaneously helping     *
*    to support the FreeRTOS project by purchasing a FreeRTOS           *
*    tutorial book, reference manual, or both:                          *
*    http://www.FreeRTOS.org/Documentation                              *
*                                                                       *
***************************************************************************

http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
the FAQ page "My application does not run, what could be wrong?".  Have you
defined configASSERT()?

http://www.FreeRTOS.org/support - In return for receiving this top quality
embedded software for free we request you assist our global community by
participating in the support forum.

http://www.FreeRTOS.org/training - Investing in training allows your team to
be as productive as possible as early as possible.  Now you can receive
FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
Ltd, and the world's leading authority on the world's leading RTOS.

http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
including FreeRTOS+Trace - an indispensable productivity tool, a DOS
compatible FAT file system, and our tiny thread aware UDP/IP stack.

http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
licenses offer ticketed support, indemnification and commercial middleware.

http://www.SafeRTOS.com - High Integrity Systems also provide a safety
engineered and independently SIL3 certified version for use in safety and
mission critical applications that require provable dependability.

1 tab == 4 spaces!
*/

/*
* 説明:
*  メモリ管理を行います.
*  関数PortMalloc, PortFreeはメモリの確保と解放を行いますが,
*  このプログラムは空きメモリをまとめます.
*  よって, メモリの断片化を防ぐことができます.
* 
* A sample implementation of pvPortMalloc() and vPortFree() that combines
* (coalescences)adjacent memory blocks as they are freed, and in so doing
* limits memory fragmentation.
*/
#include <stdlib.h>

#include "ArduinOS.h"
#include "Task.h"


// ヒープ領域の初期化. 初めにヒープ領域を使用する前に呼ぶ必要がある.
// Initialises the heap structures before their firt use.
static void HeapInit(void);

// Allocate the memory for the heap.
static unsigned char heap[CONFIG_TOTAL_HEAP_SIZE];

//
// 説明:
//  空きブロックリストの要素
//  空いている次のメモリのアドレスを持つ. また, そのサイズも保存する.
//  メモリの構造は次のようになっている
//  
//      start - freeBlock0 - freeBlock1 - end
//        |
//      struct(BlockLink) - AlignmentMemory - FreeSpace
//
//      AlignmentMemory:
//          BlockLinkとfreeSpaceの間にある
//          ; メモリをアライメントするために必要
// 
//      FreeSpace:
//          アプリケーションが自由に使えるメモリ空間
//    
//
//  Define the linked list structure.
//  This is used to link free blocks in order of their size.
struct BlockLink
{
    //次の空きブロックのポインタ
    //The next free block in the list
    struct BlockLink *nextFreeBlock;

    // These size of the free block.
    size_t blockSize;
};

typedef struct BlockLink BlockLink;

// アライメントされた構造体のサイズ
static const size_t heapStructSize = (sizeof(BlockLink) + ((size_t)(PORT_BYTE_ALIGNMENT - 1))) & ~((size_t)PORT_BYTE_ALIGNMENT_MASK);

#define HEAP_MINIMUM_BLOCK_SIZE ((size_t) (heapStructSize << 1))

#define HEAP_BITS_PER_BYTE ((size_t) 8)

// Create a couple of list links to mark the start and end of the list.
BlockLink start, *end = NULL;

// 空きメモリ容量, 断片化には関係がない
// Keep track of the number of free bytes remaining, but says nothing about fragmentation.
static size_t freeBytesRemaining = 0U;
static size_t minimumEverFreeBytesRemaining = 0U;

// メモリブロックがアプリケーション側で使用されているか確認するためのフラグ
// メモリブロックのblockSizeの最上位ビットが1の場合は使用されていることになる. 逆もしかり.
//
// Gets set to the top bit of an size_t type.  When this bit in the blockSize
// member of an BlockLink structure is set then the block belongs to the
// application.  When the bit is free the block is still part of the free heap
// space.
static size_t blockAllocatedBit = 0;

// STATIC FUNCTION ARE DEFINED AS MACRO TO MINIMIZE THE FUNCTION CALL DEPTH.

//
// 
static void InsertBlockIntoFreeList(BlockLink *blockToInsert);

void* PortMalloc(size_t wantedSize)
{
    BlockLink *block, *previousBlock, *newBlockLink;

    void *ret = NULL;

    TaskSuspendAll();
    {
        //初回にこれが呼ばれたとき, リストを初期化する.
        //If this is the first call to malloc
        //then the heap wikk require initialisation to setup the list of free blocks.
        if (end == NULL)
        {
            HeapInit();
        }

        // Check the requested block size is not so large that the top bit is
        // set.  The top bit of the block size member of the BlockLink_t structure
        // is used to determine who owns the block - the application or the
        // kernel, so it must be free.
        if ((wantedSize & blockAllocatedBit) == 0)
        {

            // The wanted size is increased so it can contain a BlockLink
            // structure in addition to the requested amount of bytes.
            if (wantedSize > 0)
            {
                wantedSize += heapStructSize;

                // Ensure that blocks are always aligned to the required number of bytes.
                //
                // Memo:
                //  マスクを用いた剰余演算について:
                //   2の乗数の余りを求める際, 以下のようにマスクを用いることで高速に計算できる.
                //
                //   割る数をBとする. 割られる数をAとする.
                //    あまり = A & (B - 1)
                //
                //   補足:
                //    割られる数から余りを引いた数Cの計算は(B - 1)をビット反転しANDをとることで求めることができる.
                //    C = A & ~(B - 1)
                // 
                if ((wantedSize & PORT_BYTE_ALIGNMENT_MASK) != 0x00)
                {
                    // Byte alignment required.
                    wantedSize += (PORT_BYTE_ALIGNMENT - (wantedSize & PORT_BYTE_ALIGNMENT_MASK));
                }
            }

            if ((wantedSize > 0) && (wantedSize <= freeBytesRemaining))
            {
                // Blocks are stored in byte order
                // - traverse the list from the start (smallest) block until one of adequate size is found.
                previousBlock = &start;
                block = start.nextFreeBlock;

                // wantedSize以上のまとまった空きを持つメモリを探す
                while ((block->blockSize < wantedSize && (block->nextFreeBlock != NULL)))
                {
                    previousBlock = block;
                    block = block->nextFreeBlock;
                }

                // If we found rhe end marker then a block of adequate size was not found.
                // 空き領域内の最後のブロックに達したときはすなわち, 十分な空き領域が見つからなかったことである.
                // 最後に達していないつまり, 空き領域を見つけたとき
                if (block != end)
                {
                    // Return the memory space - jumping over the BlockLink structure at its start.
                    ret = (void *)(((uint8_t*)previousBlock->nextFreeBlock) + heapStructSize);

                    // This block is being returned for use so must be taken out of the list of free blocks.
                    previousBlock->nextFreeBlock = block->nextFreeBlock;

                    // ブロックが要求しているものより大きい場合, ブロックを二つに分ける.
                    // If the block is larger than required it can be split into two.
                    if ((block->blockSize - wantedSize) > HEAP_MINIMUM_BLOCK_SIZE)
                    {
                        // This block is to be split into two. Create a new block
                        // following the number of bytes requested. The void cast is
                        // used to prevent byte alignment warnings from the compiler.
                        newBlockLink = (void *)(((uint8_t*)block) + wantedSize);

                        // Calculate the sizes of two blocks split from the single block.
                        newBlockLink->blockSize = block->blockSize - wantedSize;
                        block->blockSize = wantedSize;

                        // Insert the new block into the list of free blocks.
                        InsertBlockIntoFreeList((newBlockLink));
                    }

                    freeBytesRemaining -= block->blockSize;

                    if (freeBytesRemaining < minimumEverFreeBytesRemaining)
                    {
                        minimumEverFreeBytesRemaining = freeBytesRemaining;
                    }

                    // The block is being returned - it is allocated and owned
                    // by the application and has no "next" block.
                    block->blockSize |= blockAllocatedBit;
                    block->nextFreeBlock = NULL;
                }
            }
        }
    }
    TaskResumeAll();

#if(CONFIG_USE_MALLOC_FAILED_HOOK == 1)
    {
        if (ret == NULL)
        {
            extern void ApplicationMallocFailedHock(void);
            ApplicationMallocFailedHook();
        }
    }
#endif

    return ret;
}

void PortFree(void *addressToFree)
{
    uint8_t *pointer = (uint8_t*)addressToFree;
    BlockLink *link;

    if (addressToFree != NULL)
    {
        // The memory being freed will have an BlockLink structures immediately
        // before it.
        pointer -= heapStructSize;

        // This unexpected casting is to keep some compilers from
        // issuing byte alignment warnings.
        link = (void*)pointer;

        if ((link->blockSize & blockAllocatedBit) != 0)
        {
            if (link->nextFreeBlock == NULL)
            {
                // The block is being returned to the heap - it is no longer
                // allocated.
                link->blockSize &= ~blockAllocatedBit;

                TaskSuspendAll();
                {
                    // Add this block to the list of free blocks.
                    freeBytesRemaining += link->blockSize;
                    InsertBlockIntoFreeList(((BlockLink*)link));
                    
                }
                TaskResumeAll();
            }
        }
    }
}

size_t PortGetFreeHeapSize(void)
{
    return freeBytesRemaining;
}

size_t PortGetMinimumEverFreeHeapSize(void)
{
    return minimumEverFreeBytesRemaining;
}

// Portable内で宣言されているPortInitialiseBlocksに関する定義
void PortInitialiseBlocks(void)
{
    // This just exists to keep the linker quiet.
}

static void HeapInit(void)
{
    BlockLink *firstFreeBlock;
    uint8_t *alignedHeap;
    size_t address;
    size_t totalHeapSize = CONFIG_TOTAL_HEAP_SIZE;

    // Ensure the heap starts on a correctly aligned boundry.
    address = (size_t)heap;

    // ヒープ領域のアドレスがアライメントされていないとき
    // PORT_BYTE_ALIGNMENT_MASK = PORT_BYTE_ALIGNMENT - 1
    if ((address & PORT_BYTE_ALIGNMENT_MASK) != 0)
    {
        // アライメントを行う
        address += (PORT_BYTE_ALIGNMENT - 1);
        address &= ~((size_t)PORT_BYTE_ALIGNMENT_MASK);
        totalHeapSize -= address - (size_t)heap;
    }
    alignedHeap = (uint8_t *)address;

    // start is used to hold a pointer to the first item in the list of free
    // blocks. The void cast is used to prevent compiler warnings.
    start.nextFreeBlock = (void*)alignedHeap;
    start.blockSize = (size_t)0;

    // end is used to mark the end of the list of free blocks.
    address = ((size_t)alignedHeap) + totalHeapSize;
    address -= heapStructSize;
    address &= ~((size_t)PORT_BYTE_ALIGNMENT_MASK);
    end = (void *)address;
    end->blockSize = 0;
    end->nextFreeBlock = NULL;

    // To start with there is a signal free block that is sized to make up the
    // entire heap space.
    firstFreeBlock = (void*)alignedHeap;
    firstFreeBlock->blockSize = address - (size_t)firstFreeBlock;
    firstFreeBlock->nextFreeBlock = end;


    // Only one block exists - and it covers the entire usable heap space.
    minimumEverFreeBytesRemaining = firstFreeBlock->blockSize;
    freeBytesRemaining = firstFreeBlock->blockSize;

    // Work out the position of the top bit in a size_t variable.
    blockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * HEAP_BITS_PER_BYTE) - 1);
}

static void InsertBlockIntoFreeList(BlockLink *blockToInsert)
{
    BlockLink *iterator;
    uint8_t *pointer;

    // 挿入されるアドレスの次のアドレスがイテレータのアドレス以上になるまで繰り返す.
    // アドレスの順番は以下のようになる.
    //  (iterator) - (blockToInsert) - (iterator->nextFreeBlock)
    //
    // Iterate through the list until a block is found that has a higher address
    // than the block being inserted.
    for (iterator = &start; iterator->nextFreeBlock < blockToInsert; iterator = iterator->nextFreeBlock)
    {
        // ここでは何もしない.
        // イテレートするのみ.
        // Nothing to do here, just iterate to the right position.
    }

    // iterator と blockToInsert の融合
    //
    // Do the block being inserted, and the block it is being inserted after
    // make a contiguous block of memory?
    pointer = (uint8_t *)iterator;
    if ((pointer + iterator->blockSize) == (uint8_t *)blockToInsert)
    {
        iterator->blockSize += blockToInsert->blockSize;
        blockToInsert = iterator;
    }

    // blockToInsert と iterator->nextFreeBlock との融合
    //
    // Do the block being inserted, and the block it is being inserted before
    // make a contiguous block of memory?
    pointer = (uint8_t *)blockToInsert;
    if ((pointer + blockToInsert->blockSize) == (uint8_t *)iterator->nextFreeBlock)
    {
        if (iterator->nextFreeBlock != end)
        {
            // Form one big block from the two blocks.
            blockToInsert->blockSize += iterator->nextFreeBlock->blockSize;
            blockToInsert->nextFreeBlock = iterator->nextFreeBlock->nextFreeBlock;
        }
        else
        {
            blockToInsert->nextFreeBlock = end;
        }
    }
    else
    {
        blockToInsert->nextFreeBlock = iterator->nextFreeBlock;
    }

    // If the block being inserted plugged a gab, so was merged with the block
    // before and the block after, then it's nextFreeBlock pointer will have
    // already been set, and should not be set here as that would make it point
    // to itself.
    if (iterator != blockToInsert)
    {
        iterator->nextFreeBlock = blockToInsert;
    }
}