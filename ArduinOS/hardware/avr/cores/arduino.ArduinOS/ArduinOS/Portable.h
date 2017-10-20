/*
// ArduinOS
//
// ArduinOS�Ƃ�, ���A���^�C��OS(RTOS)�𗝉����邽�߂�, ���Ƃ��Ƃ���FreeRTOS����
// �K�v�ȋ@�\�𔲂��o��, Arduino�̊J�����Ŏg�p�ł���悤�ɂ������̂ł�.
//
// GNU General Public License(ver2) ���K�p����Ă��܂�.
//
// ArduinOS�Ɋւ���ڂ��������͈ȉ��̃y�[�W���Q�Ƃ��Ă�������.
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


#ifndef PORTABLE_H
#define PORTABLE_H

#include "PortMacro.h"

#if PORT_BYTE_ALIGNMENT == 8
    #define PORT_BYTE_ALIGNMENT_MASK (0x0007)
#endif

#if PORT_BYTE_ALIGNMENT == 4
    #define PORT_BYTE_ALIGNMENT_MASK (0x0003)
#endif

#if PORT_BYTE_ALIGNMENT == 2
    #define PORT_BYTE_ALIGNMENT_MASK (0x0001)
#endif

#if PORT_BYTE_ALIGNMENT == 1
    #define PORT_BYTE_ALIGNMENT_MASK (0x0000)
#endif

#ifndef PORT_BYTE_ALIGNMENT_MASK
    #error "Invalid PORT_BYTE_ALIGNMENT definition"
#endif


#ifdef __cplusplus
extern "C" {
#endif

    // Setup the stack of anew task so it is readyto be placed under the
    // scheduler control. The registers have to be placed on the stack in
    // the order that the port expects to find them.
    PortStackType *PortInitialiseStack(PortStackType *topOfStack, TaskCode code, void *parameters);

    // Map to the memory management routines required for the port
    void* PortMalloc(size_t size);
    void PortFree(void *pv);
    void PortInitialiseBlocks(void);
    size_t PortGetFreeHeapSize(void);

    // Setup the hardware rady for the scheduler to take control. This generally 
    // sets up a tick interrupt and sets timers for the correct tick frequency.
    PortBaseType PortStartScheduler(void);

    void PortEndScheduler(void);


#ifdef __cplusplus
}
#endif
#endif
