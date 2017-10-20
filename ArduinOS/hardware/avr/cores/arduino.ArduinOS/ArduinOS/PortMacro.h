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


// ---------------------------------------------------------
// Port specific definitions.
// ---------------------------------------------------------


#ifndef PORTMACRO_H
#define PORTMACRO_H

// C言語でコンパイルするように宣言
// 関数が正しくリンクされるようにするため.
#ifdef __cplusplus
extern "C" {
#endif

    // --- Type definitions --------------------------------------------
#define PortChar char
#define PortFloat float
#define PortDouble double
#define PortLong long
#define PortShort int
#define PortStackType unsigned PortChar
#define PortBaseType char

#if(CONFIG_USE_16BIT_TICKS == 1)
    typedef unsigned PortShort PortTickType;
#define PORT_MAX_DELAY (PortTickType) 0xffff
#else
    typedef unsigned PortLong PortTickType;
#define PORT_MAX_DELAY (PortTickType) 0xffffffff
#endif
    // -------------------------------------------------------------------

    // --- Critical section management -----------------------------------
#define PortEnterCritical() \
    asm volatile ("in       __tmp_reg__, __SREG__"::);\
    asm volatile ("cli":: );\
    asm volatile ("push     __tmp_reg__"::)

#define PortExitCritical() \
    asm volatile ("pop      __tmp_reg__"::);\
    asm volatile ("out      __SREG__, __tmp_reg__"::)

#define PortDisableInterrupts() \
    asm volatile ("cli"::);

#define PortEnableInterrupts() \
    asm volatile ("sei"::);
// -------------------------------------------------------------------

// --- Architecture specifics ----------------------------------------


//
// スタックでの要素を積む方向
// 負のとき, スタックへ要素を積まれたときスタックの先頭アドレスが低くなる.
// 正のとき, スタックへ要素を積まれたときスタックの先頭アドレスが高くなる.
//
#define PORT_STACK_GROWTH (-1)

// タイマー割り込み周期. 1tickあたりの時間(ms)
// Memo:
//  この時点でCONFIG_TICK_RATE_HZが定義されていなくても,
//  アプリケーション側でPORT_TICK_RATE_MSを使用しない限り, エラーは出ない.
//
#define PORT_TICK_RATE_MS ((PortTickType) 1000 / CONFIG_TICK_RATE_HZ)

#define PORT_BYTE_ALIGNMENT 1
#define PortNop() asm volatile ("nop");
// -------------------------------------------------------------------

// ---Kernel utilities -----------------------------------------------
    extern void PortYield(void) __attribute__((naked));

// -------------------------------------------------------------------

    // Task function macros
#define PortTaskFunctionProto(function, parameters) void function(void *parameters)
#define PortTaskFunction(function, parameters) void function(void *parameters)


#ifdef __cplusplus
}
#endif

#endif