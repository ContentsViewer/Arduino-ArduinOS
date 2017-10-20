
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

#ifndef STACK_MACROS_H
#define STACK_MACROS_H

/*
// タスクが個々に占有するメモリ空間を超えた, もしくは
// 超える可能性があるとき, スタックオバーフローフック関数を呼び出します.
//
// CONFIG_CHECK_FOR_STACK_OVERFLOW を1に設定すると, 現在のスタック状態
// を確認します. CONFIG_CHECK_FOR_STACK_OVERFLOW を1より大きく設定すると,
// スタック領域の上部数バイトがタスク作成時に埋められた数値が上書きされていないかどうか
// を確認します(上書きされている場合, スタック領域上部までスタックが
// 使用されたことになり, 領域を超える可能性もしくは超えたことになります.)
// 必ずしも超えていると保証できないことに注意してください.
//
// 検知法1より2の方が厳密です. 検知法1では, 領域を超えてpushされ領域内までpopされた場合を
// 検知できませんが, 検知法2では可能です. 領域を超えてpushされた時点でタスク作成時に埋められた
// ビットパターンを上書きしてしまい検知法2ではこれを検知するからです. 
// いわゆる使用された形跡を検知します.
//
// このスタックオバーフローの確認はタスク切り替え時(TaskSwitchContext())に行われます.
//
*/
/*
* Call the stack overflow hook function if the stack of the task being swapped
* out is currently overflowed, or looks like it might have overflowed in the
* past.
*
* Setting CONFIG_CHECK_FOR_STACK_OVERFLOW to 1 will cause the macro to check
* the current stack state only - comparing the current top of stack value to
* the stack limit.  Setting CONFIG_CHECK_FOR_STACK_OVERFLOW to greater than 1
* will also cause the last few stack bytes to be checked to ensure the value
* to which the bytes were set when the task was created have not been
* overwritten.  Note this second test does not guarantee that an overflowed
* stack will always be recognised.
*/


#if(CONFIG_CHECK_FOR_STACK_OVERFLOW == 0)
    // スタックオバーフローを確認しない.
    // ArduinOSConfig.h is not set to check for stack overflows.

    #define TaskFirstCheckForStackOverflow()
    #define TaskSecondCheckForStackOverflow()

#endif

#if(CONFIG_CHECK_FOR_STACK_OVERFLOW == 1)
    // 一番目の方法による確認を行うが二番目の確認は行わない.
    // ArduinOSConfig.h is only set to use the first method of 
    // overflow checking.
    #define TaskSecondCheckForStackOverflow()
#endif

#if ((CONFIG_CHECK_FOR_STACK_OVERFLOW > 0) && (PORT_STACK_GROWTH < 0))
    // Only the current stack state is to be checked.
    // 現在のスタック状態のみ確認
    #define TaskFirstCheckForStackOverflow()                                                \
    {                                                                                       \
        /* Is the currently saved stack pointer within the stack limit? */                  \
        /* 現在のスタックポインタが, 占有するスタック領域を超えていないか? */               \
        if (currentTCB->topOfStack <= currentTCB->stack)                                    \
        {                                                                                   \
            ApplicationStackOverflowHook((TaskHandle)currentTCB, currentTCB->taskName);     \
        }                                                                                   \
    }

#endif

#if ((CONFIG_CHECK_FOR_STACK_OVERFLOW > 0) && (PORT_STACK_GROWTH > 0))
    // Only the current stack state is to be checked.
    // 現在のスタック状態のみ確認
    #define TaskFirstCheckForStackOverflow()                                                \
    {                                                                                       \
        /* Is the currently saved stack pointer within the stack limit? */                  \
        /* 現在のスタックポインタが, 占有するスタック領域を超えていないか? */               \
        if (currentTCB->topOfStack >= currentTCB->endOfStack)                               \
        {                                                                                   \
            ApplicationStackOverflowHook((TaskHandle)currentTCB, currentTCB->taskName);     \
        }                                                                                   \
    }

#endif

#if((CONFIG_CHECK_FOR_STACK_OVERFLOW > 1) && (PORT_STACK_GROWTH < 0))
    // スタック領域上部でスタックが使用されたかどうか調べる
    #define TaskSecondCheckForStackOverflow()                                                                       \
    {                                                                                                               \
        static const unsigned char EXPECTED_STACK_BYTES[] = {                                                       \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE                  \
        };                                                                                                          \
                                                                                                                    \
        /* Has the extremity of the task stack ever been written over? */                                           \
        /* スタック領域上部でスタックが使用されているか? */                                                         \
        /* タスク作成時に埋めたビットパターンが上書きされているか確認する. */                                       \
        if (memcmp((void *)currentTCB->stack, (void *)EXPECTED_STACK_BYTES, sizeof(EXPECTED_STACK_BYTES)) != 0)     \
        {                                                                                                           \
            ApplicationStackOverflowHook((TaskHandle)currentTCB, currentTCB->taskName);                             \
        }                                                                                                           \
    }
#endif


#if((CONFIG_CHECK_FOR_STACK_OVERFLOW > 1) && (PORT_STACK_GROWTH > 0))
    // スタック領域上部でスタックが使用されたかどうか調べる
    #define TaskSecondCheckForStackOverflow()                                                                       \
    {                                                                                                               \
        char *endOfStack = (char *)currentTCB->endOfStack;                                                          \
                                                                                                                    \
        static const unsigned char EXPECTED_STACK_BYTES[] = {                                                       \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE,                 \
            TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE, TASK_STACK_FILL_BYTE                  \
        };                                                                                                          \
                                                                                                                    \
        endOfStack -= sizeof(EXPECTED_STACK_BYTES);                                                                 \
                                                                                                                    \
        /* Has the extremity of the task stack ever been written over? */                                           \
        /* スタック領域上部でスタックが使用されているか? */                                                         \
        /* タスク作成時に埋めたビットパターンが上書きされているか確認する. */                                       \
        if (memcmp((void *)endOfStack, (void *)EXPECTED_STACK_BYTES, sizeof(EXPECTED_STACK_BYTES)) != 0)            \
        {                                                                                                           \
            ApplicationStackOverflowHook((TaskHandle)currentTCB, currentTCB->taskName);                             \
        }                                                                                                           \
    }
#endif

#endif