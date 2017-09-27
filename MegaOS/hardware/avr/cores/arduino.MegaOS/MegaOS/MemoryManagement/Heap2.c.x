
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

//
// 説明:
//  メモリ管理を行います.
//  関数PortMalloc, PortFreeはメモリの確保と解放を行いますが,
//  空きメモリをまとめることはしません. 
//  よって, メモリの断片化が起こる可能性があります.
// 
//  A sample implementation of PortMalloc() and PortFree() that permits
//  allocated blocks to be freed, but does not combine adjacent free blocks
//  into a signal larger block (and so will fragment memory).
//
#include <stdlib.h>

#include "MegaOS.h"
#include "Task.h"

//A few bytes might be lost to byte aligning the heap start address.
#define CONFIG_ADJUSTED_HEAP_SIZE (CONFIG_TOTAL_HEAP_SIZE - PORT_BYTE_ALIGNMENT)


//ヒープ領域の初期化. 初めにヒープ領域を使用する前に呼ぶ必要がある.
//Initialises the heap structures before their firt use.
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
static const unsigned short heapStructSize = (sizeof(BlockLink) + PORT_BYTE_ALIGNMENT
    - (sizeof(BlockLink) % PORT_BYTE_ALIGNMENT));

#define HEAP_MINIMUM_BLOCK_SIZE ((size_t) (heapStructSize * 2))

// Create a couple of list links to mark the start and end of the list.
BlockLink start, end;

// 空きメモリ容量, 断片化には関係がない
// Keep track of the number of free bytes remaining, but says nothing about fragmentation.
static size_t freeBytesRemaining = CONFIG_ADJUSTED_HEAP_SIZE;

// STATIC FUNCTION ARE DEFINED AS MACRO TO MINIMIZE THE FUNCTION CALL DEPTH.

//
// Insert a block into the list of free blocks - which is ordered by size of
// the block. Small blocks at the start of the list and large blocks at the end
// of the list.
//
#define InsertBlockIntoFreeList(blockToInsert)                                    \
{                                                                                 \
    BlockLink *iterator;                                                          \
    size_t blockSize;                                                             \
    /* Iterate through the list until a block is found that has a larger size */  \
    /* than the block we are inserting. */                                        \
    for(iterator = &start                                                         \
        ; iterator->nextFreeBlock->blockSize < blockSize                          \
        ; iterator = iterator->nextFreeBlock)                                     \
    {                                                                             \
        /* There is nothing to do here - just iterate to the current position */  \
    }                                                                             \
                                                                                  \
    /* Update the list to include the block being inserted in the current */      \
    /* Position */                                                                \
    blockToInsert->nextFreeBlock = iterator->nextFreeBlock;                       \
    iterator->nextFreeBlock = blockToInsert;                                      \
}

void* PortMalloc(size_t wantedSize)
{
    BlockLink *block, *previousBlock, *newBlockLink;

    static PortBaseType heapHasBeenInitialized = PD_FALSE;
    void *ret = NULL;

    TaskSuspendAll();
    {
        //初回にこれが呼ばれたとき, リストを初期化する.
        //If this is the first call to malloc
        //then the heap wikk require initialisation to setup the list of free blocks.
        if (heapHasBeenInitialized == PD_FALSE)
        {
            HeapInit();
            heapHasBeenInitialized = PD_TRUE;
        }

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
            //    商の計算は(B - 1)をビット反転しANDをとることで求めることができる.
            //    商 = A & ~(B - 1)
            //   
            if (wantedSize & PORT_BYTE_ALIGNMENT_MASK)
            {
                // Byte alignment required.
                wantedSize += (PORT_BYTE_ALIGNMENT - (wantedSize & PORT_BYTE_ALIGNMENT_MASK));
            }
        }

        if ((wantedSize > 0) && (wantedSize < CONFIG_ADJUSTED_HEAP_SIZE))
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
            if (block != &end)
            {
                // Return the memory space - jumping over the BlockLink structure at its start.
                ret = (void *)(((unsigned char*)previousBlock->nextFreeBlock) + heapStructSize);

                // This block is being returned for use so must be taken out of the list of free blocks.
                previousBlock->nextFreeBlock = block->nextFreeBlock;

                // ブロックが要求しているものより大きい場合, ブロックを二つに分ける.
                // If the block is larger than required it can be split into two.
                if ((block->blockSize - wantedSize) > HEAP_MINIMUM_BLOCK_SIZE)
                {
                    // This block is to be split into two. Create a new block
                    // following the number of bytes requested. The void cast is
                    // used to prevent byte alignment warnings from the compiler.
                    newBlockLink = (void *)(((unsigned char*)block) + wantedSize);

                    // Calculate the sizes of two blocks split from the single block.
                    newBlockLink->blockSize = block->blockSize - wantedSize;
                    block->blockSize = wantedSize;

                    // Insert the new block into the list of free blocks.
                    InsertBlockIntoFreeList((newBlockLink));
                }

                freeBytesRemaining -= block->blockSize;
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

void PortFree(void *pv)
{
    unsigned char *puc = (unsigned char*)pv;
    BlockLink *link;

    if (pv != NULL)
    {
        // The memory being freed will have an BlockLink structures immediately
        // before it.
        puc -= heapStructSize;

        // This unexpected casting is to keep some compilers from
        // issuing byte alignment warnings.
        link = (void*)puc;

        TaskSuspendAll();
        {
            // Add this block to the list of free blocks.
            InsertBlockIntoFreeList(((BlockLink*)link));
            freeBytesRemaining += link->blockSize;
        }
        TaskResumeAll();
    }
}

size_t PortGetFreeHeapSize(void)
{
    return freeBytesRemaining;
}

// Portable内で宣言されているPortInitialiseBlocksに関する定義
void PortInitialiseBlocks(void)
{
    // This just exists to keep the linker quiet.
}

static void HeapInit(void)
{
    BlockLink *firstFreeBlock;
    unsigned char *alignedHeap;

    // Ensure the heap starts on a correctly aligned boundry.
    alignedHeap = (unsigned char*) (((PortPointerSizeType) &heap[PORT_BYTE_ALIGNMENT])
        & ((PortPointerSizeType) ~PORT_BYTE_ALIGNMENT_MASK));

    // start is used to hold a pointer to the first item in the list of free
    // blocks. The void cast is used to prevent compiler warnings.
    start.nextFreeBlock = (void*)alignedHeap;
    start.blockSize = (size_t)0;

    // end is used to mark the end of the list of free blocks.
    end.blockSize = CONFIG_ADJUSTED_HEAP_SIZE;
    end.nextFreeBlock = NULL;

    // To start with there is a signal free block that is sized to make up the
    // entire heap space.
    firstFreeBlock = (void*)alignedHeap;
    firstFreeBlock->blockSize = CONFIG_ADJUSTED_HEAP_SIZE;
    firstFreeBlock->nextFreeBlock = &end;
}