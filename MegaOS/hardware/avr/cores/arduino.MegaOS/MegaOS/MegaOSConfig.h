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
FreeRTOS.org V5.3.0 - Copyright (C) 2003-2009 Richard Barry.

This file is part of the FreeRTOS.org distribution.

FreeRTOS.org is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License (version 2) as published
by the Free Software Foundation and modified by the FreeRTOS exception.
**NOTE** The exception to the GPL is included to allow you to distribute a
combined work that includes FreeRTOS.org without being obliged to provide
the source code for any proprietary components.  Alternative commercial
license and support terms are also available upon request.  See the
licensing section of http://www.FreeRTOS.org for full details.

FreeRTOS.org is distributed in the hope that it will be useful,	but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along
with FreeRTOS.org; if not, write to the Free Software Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA  02111-1307  USA.


***************************************************************************
*                                                                         *
* Get the FreeRTOS eBook!  See http://www.FreeRTOS.org/Documentation      *
*                                                                         *
* This is a concise, step by step, 'hands on' guide that describes both   *
* general multitasking concepts and FreeRTOS specifics. It presents and   *
* explains numerous examples that are written using the FreeRTOS API.     *
* Full source code for all the examples is provided in an accompanying    *
* .zip file.                                                              *
*                                                                         *
***************************************************************************

1 tab == 4 spaces!

Please ensure to read the configuration and relevant port sections of the
online documentation.

http://www.FreeRTOS.org - Documentation, latest information, license and
contact details.

http://www.SafeRTOS.com - A version that is certified for use in safety
critical systems.

http://www.OpenRTOS.com - Commercial support, development, porting,
licensing and training services.
*/


#ifndef MEGA_OS_CONFIG_H
#define MEGA_OS_CONFIG_H

//#define __AVR_ATmega328P__

#if defined(__AVR_ATmega328P__)
    #include "MegaOSConfigAtmega328P.h"

#elif defined(__AVR_ATmega2560__)
    #include "MegaOSConfigAtmega2560.h"
#elif defined(__AVR_ATmega168__)
    #include "MegaOSConfigAtmega168.h"
#else
    #error "Device is not supportrd by MegaOS"
#endif

#ifndef CONFIG_USE_PRERMPTION
    #define CONFIG_USE_PREEMPTION 1
#endif

#ifndef CONFIG_USE_IDLE_HOOK
    #define CONFIG_USE_IDLE_HOOK 0
#endif

#ifndef CONFIG_USE_TICK_HOOK
    #define CONFIG_USE_TICK_HOOK 1
#endif

// クロック周波数ごとのTimer割り込み周波数:
//  クロック周波数: 16Mhz
//   Timer割り込み周波数: 16Mhz / (256 * 64) = 976.5625 Hz
#ifndef CONFIG_TICK_RATE_HZ

    #if F_CPU >= 20000000L
        // 20Mhzで動作するとき
        #define CONFIG_TICK_RATE_HZ ((PortTickType)1250)
    #elif F_CPU >= 16000000L
        // 16Mhzで動作するとき
        #define CONFIG_TICK_RATE_HZ ((PortTickType)1000)
    #else
        // 8Mhzで動作するとき
        #define CONFIG_TICK_RATE_HZ ((PortTickType)500)
    #endif
#endif

#ifndef CONFIG_SETUP_STACK_SIZE
    #define CONFIG_SETUP_STACK_SIZE (CONFIG_MINIMAL_STACK_SIZE)
#endif

#ifndef CONFIG_LOOP_STACK_SIZE
    #define CONFIG_LOOP_STACK_SIZE (CONFIG_MINIMAL_STACK_SIZE)
#endif

#endif