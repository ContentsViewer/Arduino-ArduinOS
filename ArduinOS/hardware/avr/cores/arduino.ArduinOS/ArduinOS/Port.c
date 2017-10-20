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


/*
// AVRマイコン専用ハードウェア操作モジュール
//
//
// 
//
*/
#include <avr/interrupt.h>

#include "ArduinOS.h"
#include "Task.h"
#include "wiring_private.h"


// ------------------------------------------------------------------------
// Implementation of function defined in Portable.h for the AVR port.
// ------------------------------------------------------------------------

//
// Start tasks with interrupts enables.
// *** Memo *********************************************
//  SREG:
//      ステータスレジスタ
//      0x80(1000 0000) 7bit目が全割り込みフラグである.
// ******************************************************
#define PORT_FLAGS_INT_ENABLED ((PortStackType)0x80)


// We require the address of the currentTCB variable, but don't want to know
// any details of its type.
typedef void TaskControlBlock;
extern volatile TaskControlBlock * volatile currentTCB;

// 汎用レジスタの保存, スタックポインタをTCBに保存
//
// まずやることはフラグの保存(ステータスレジスタの保存)そのあと割り込み停止である.
//
// Macro to save all the general purpose registers, the save the stack pointer into the TCB.
// 
// The first thing we do is save the flags then disable interrupts.
// This is to guard our stack against having a contex switch interrupt after we have already
// pushed the resisters onto the stack - causing the 32 registers to be on the stack twice.
//
// r1 is set to zero as the compiler expects it to be thus, 
// however some of the math routines make use of r1.
//
// The interupts will have been disabled during the call to PortSaveContext()
// so we need not worry about reading/writing to the stack pointer.
// 
// *** Memo ***************************************************
// x: Xレジスタ(R26 : R27)
// R26: Xレジスタ下位バイト
// R27: Xレジスタ上位バイト
// R28: Yレジスタ下位バイト
// R29: Yレジスタ上位バイト
// R30: Zレジスタ下位バイト
// R31: Zレジスタ上位バイト
// SP_H: スタックポインタ上位バイト(0x3e)
// SP_L: スタックポインタ下位バイト(0x3d)
// SREG: ステータスレジスタ
// ************************************************************
inline void PortSaveContext(void);
inline void PortSaveContext(void)
{
    asm volatile(
        "push   r0                          \n\t"
        "in     r0, __SREG__                \n\t"
        "cli                                \n\t"
        "push   r0                          \n\t"
        );

#if defined(__AVR_HAVE_RAMPZ__)
    // have RAMPZ Extended Z-pointer Register for ELPM/SPM
    // the uC have extend program memory
    // 0x3b --> RAMPZ
    // 0x3c --> EIND
    //
    // RAMPZレジスタ, EINDレジスタを持つCPUには, これらレジスタの値も保存する.
    //
    // memo:
    //  AVRマイコンでは, 年々メモリの容量が増加しており, アドレス16ビットではメモリすべてのアドレス
    //  を示すことができなくなってきた. このような問題に対処するため,
    //  Zレジスタを拡張するRAMPZ, EINDレジスタが導入された.
    asm volatile(
        "in     r0, 0x3b                    \n\t"
        "push   r0                          \n\t"
        "in     r0, 0x3c                    \n\t"
        "push   r0                          \n\t"
        );
#endif
    asm volatile(
        "push   r1                          \n\t"
        "clr    r1                          \n\t"
        "push   r2                          \n\t"
        "push   r3                          \n\t"
        "push   r4                          \n\t"
        "push   r5                          \n\t"
        "push   r6                          \n\t"
        "push   r7                          \n\t"
        "push   r8                          \n\t"
        "push   r9                          \n\t"
        "push   r10                         \n\t"
        "push   r11                         \n\t"
        "push   r12                         \n\t"
        "push   r13                         \n\t"
        "push   r14                         \n\t"
        "push   r15                         \n\t"
        "push   r16                         \n\t"
        "push   r17                         \n\t"
        "push   r18                         \n\t"
        "push   r19                         \n\t"
        "push   r20                         \n\t"
        "push   r21                         \n\t"
        "push   r22                         \n\t"
        "push   r23                         \n\t"
        "push   r24                         \n\t"
        "push   r25                         \n\t"
        "push   r26                         \n\t"
        "push   r27                         \n\t"
        "push   r28                         \n\t"
        "push   r29                         \n\t"
        "push   r30                         \n\t"
        "push   r31                         \n\t"
        "lds    r26, currentTCB             \n\t"
        "lds    r27, currentTCB + 1         \n\t"
        "in     r0, 0x3d                    \n\t"
        "st     x+, r0                      \n\t"
        "in     r0, 0x3e                    \n\t"
        "st     x+, r0                      \n\t"
        );

}

// PortSaveContext()と逆のことをする.
// 割り込みはPortSaveContext()で停止済み.
//
// Opposite to PortSaveContext().
// Interrupts will have been disabled during the context save so we can write to the stack pointer.
inline void PortRestoreContext(void);
inline void PortRestoreContext(void)
{
    asm volatile(
        "lds    r26, currentTCB             \n\t"
        "lds    r27, currentTCB + 1         \n\t"
        "ld     r28, x+                     \n\t"
        "out    __SP_L__, r28               \n\t"
        "ld     r29, x+                     \n\t"
        "out    __SP_H__, r29               \n\t"
        "pop    r31                         \n\t"
        "pop    r30                         \n\t"
        "pop    r29                         \n\t"
        "pop    r28                         \n\t"
        "pop    r27                         \n\t"
        "pop    r26                         \n\t"
        "pop    r25                         \n\t"
        "pop    r24                         \n\t"
        "pop    r23                         \n\t"
        "pop    r22                         \n\t"
        "pop    r21                         \n\t"
        "pop    r20                         \n\t"
        "pop    r19                         \n\t"
        "pop    r18                         \n\t"
        "pop    r17                         \n\t"
        "pop    r16                         \n\t"
        "pop    r15                         \n\t"
        "pop    r14                         \n\t"
        "pop    r13                         \n\t"
        "pop    r12                         \n\t"
        "pop    r11                         \n\t"
        "pop    r10                         \n\t"
        "pop    r9                          \n\t"
        "pop    r8                          \n\t"
        "pop    r7                          \n\t"
        "pop    r6                          \n\t"
        "pop    r5                          \n\t"
        "pop    r4                          \n\t"
        "pop    r3                          \n\t"
        "pop    r2                          \n\t"
        "pop    r1                          \n\t"
        "pop    r0                          \n\t"
        );

#if defined(__AVR_HAVE_RAMPZ__)
    // have RAMPZ Extended Z-pointer Register for ELPM/SPM
    // the uC have extend program memory
    // 0x3b --> RAMPZ
    // 0x3c --> EIND
    asm volatile(
        "out    0x3c, r0                    \n\t"
        "pop    r0                          \n\t"
        "out    0x3b, r0                    \n\t"
        "pop    r0                          \n\t"
        );
#endif

    asm volatile(
        "out    __SREG__, r0                \n\t"
        "pop    r0                          \n\t"
        );
}

// Perfome hardware setup to enable ticks from timer 0, compare match A.
static void SetupTimerInterrupt( void );

PortStackType *PortInitialiseStack(PortStackType *topOfStack, TaskCode code, void *parameters)
{
    // プログラムカウンタの最大カウント数すなわちメモリの最大番地数を保持できる
    // 変数を用意する.
#if defined(__AVR_HAVE_RAMPZ__) || defined(__AVR_3_BYTE_PC__)
    // プログラムカウンタの最大カウント数17bit, 22bit, 24bit
    // ATMega2560 have 17bit Program Counter register
    // Other future uControler can have up 22, or 24bits.
    unsigned PortLong address;
#else
    // プログラムカウンタの最大カウント数16bit
    // over ATmega have 16bit Program Counter register
    unsigned PortShort address;
#endif

    // Place a few bytes of known values on the bottom of stack.
    // This is just useful for debugging.
    *topOfStack = 0x11; // 0001 0001
    topOfStack--;
    *topOfStack = 0x22; // 0010 0010
    topOfStack--;
    *topOfStack = 0x33; // 0011 0011
    topOfStack--;

    // Simulate how the stack would look after a call to PortYield() generated by
    // the compiler.
#if defined(__AVR_HAVE_RAMPZ__) || defined(__AVR_3_BYTE_PC__)
    // Implement normal stack initialisation but with PortLong instead of PortShort
    address = (unsigned PortLong) code;
    *topOfStack = (PortStackType)(address & (unsigned PortLong)0x000000ff);
    topOfStack--;

    address >>= 8;
    *topOfStack = (PortStackType)(address & (unsigned PortLong)0x000000ff);
    topOfStack--;

    // Implemented the 3byte addressing
    address >>= 8;
    *topOfStack = (PortStackType)(address & (unsigned PortLong)0x000000ff);
    topOfStack--;

// Normal initialisation for overATmega
#else
    // The start of the task code will be popped off the stack last, so place
    // it on first.
    address = (unsigned PortShort)code;

    // 下位ビット, 上位ビットの順に置く
    *topOfStack = (PortStackType)(address & (unsigned PortShort)0x00ff);
    topOfStack--;

    address >>= 8;
    *topOfStack = (PortStackType)(address & (unsigned PortShort)0x00ff);
    topOfStack--;
#endif

    // Next simulate the stack as if after a call to PortSaveContext().
    // PortSaveContext places the flags on the stack immediately after r0
    // to ensure the interrupts get disabled as soon as possible, and so ensureing
    // the stack use is minimal should a context switch interrupt occur.
    *topOfStack = (PortStackType)0x00;  // R0
    topOfStack--;
    *topOfStack = PORT_FLAGS_INT_ENABLED;
    topOfStack--;

#if defined(__AVR_HAVE_RAMPZ__)
    // have RAMPZ Extended Z-pointer Register for WLPM/SPM
    // the uC have extend program ,e,ory

    // The ATmage2560 has two more regster that we are saving 
    // The EIND and RAMPZ and we are going to initialize
    // this registers to o (default initial values)
    *topOfStack = (PortStackType)0x00;  // EIND
    topOfStack--;
    *topOfStack = (PortStackType)0x00;  // RAMPZ
    topOfStack--;
#endif

    // Now the remaing registers.
    // The compiler expects R1 to be 0.
    *topOfStack = (PortStackType)0x00;  // R1
    topOfStack--;
    *topOfStack = (PortStackType)0x02;  // R2
    topOfStack--;
    *topOfStack = (PortStackType)0x03;  // R3
    topOfStack--;
    *topOfStack = (PortStackType)0x04;  // R4
    topOfStack--;
    *topOfStack = (PortStackType)0x05;  // R5
    topOfStack--;
    *topOfStack = (PortStackType)0x06;  // R6
    topOfStack--;
    *topOfStack = (PortStackType)0x07;  // R7
    topOfStack--;
    *topOfStack = (PortStackType)0x08;  // R8
    topOfStack--;
    *topOfStack = (PortStackType)0x09;  // R9
    topOfStack--;
    *topOfStack = (PortStackType)0x10;  // R10
    topOfStack--; 
    *topOfStack = (PortStackType)0x11;  // R11
    topOfStack--; 
    *topOfStack = (PortStackType)0x12;  // R12
    topOfStack--;
    *topOfStack = (PortStackType)0x13;  // R13
    topOfStack--;
    *topOfStack = (PortStackType)0x14;  // R14
    topOfStack--;
    *topOfStack = (PortStackType)0x15;  // R15
    topOfStack--;
    *topOfStack = (PortStackType)0x16;  // R16
    topOfStack--;
    *topOfStack = (PortStackType)0x17;  // R17
    topOfStack--;
    *topOfStack = (PortStackType)0x18;  // R18
    topOfStack--;
    *topOfStack = (PortStackType)0x19;  // R19
    topOfStack--;
    *topOfStack = (PortStackType)0x20;  // R20
    topOfStack--;
    *topOfStack = (PortStackType)0x21;  // R21
    topOfStack--;
    *topOfStack = (PortStackType)0x22;  // R22
    topOfStack--;
    *topOfStack = (PortStackType)0x23;  // R23
    topOfStack--;

    // Place the parameter on the stack in the expected location.
    address = (unsigned PortShort) parameters;
    *topOfStack = (PortStackType)(address & (unsigned PortShort) 0x00ff);
    topOfStack--;

    address >>= 8;
    *topOfStack = (PortStackType)(address & (unsigned PortShort) 0x00ff);
    topOfStack--;

    *topOfStack = (PortStackType)0x26;  // R26 X
    topOfStack--;
    *topOfStack = (PortStackType)0x27;  // R27
    topOfStack--;
    *topOfStack = (PortStackType)0x28;  // R28 Y
    topOfStack--;
    *topOfStack = (PortStackType)0x29;  // R29
    topOfStack--;
    *topOfStack = (PortStackType)0x30;  // R30 Z
    topOfStack--;
    *topOfStack = (PortStackType)0x31;  // R31
    topOfStack--;

    return topOfStack;
}
PortBaseType PortStartScheduler( void )
{
    // タイマー設定
    // Setup the hardware to generate the tick.
    SetupTimerInterrupt();

    // Restore the contex of the first task that is going to run.
    PortRestoreContext();

    // Simulate a function call end as generated by the compiler.
    // We will now jump to the start of the task the context of which we have just restored.
    asm volatile ( "ret" );

    // Should not get here.
    return PD_TRUE;
}

void PortEndScheduler(void)
{
    // It is unlikely that the AVR port will get stopped. 
    // If required simply disable the tick interrupt here.
}

// Manual context switch. The first thing we do is save the registers
// so we can use a naked attribute.
//
// --- Memo ---
// naked:
// This attribute is available on the ARM, AVR, MCORE, MSP430, NDS32, RL78, RX
// and SPU ports. It allows the compiler to construct the requisite function
// declaration, while allowing the bdy of the function to be assembly code.
// The specified function will not have prologue / epilogue sequences generated
// by the compiler. Only Basic asm statements can safely be included in naked functions
// (see Basic Asm). While using Extended asm or a mixture of Basic asm and "C" code
// may appear to work, they cannot be depended upon to work reliably and are not supported.
// 
// コンパイラに関数作成に必要最低限の宣言を書かせる.
// 関数の中身はassemblyコードで書くことができる.
// その際, prologueコードとepilogueコードが生成されない.(retも生成されない).
//
void PortYield(void) __attribute__((naked));
void PortYield(void)
{
    PortSaveContext();
    TaskSwitchContext();
    PortRestoreContext();

    asm volatile("ret");
}

//
// Context switch function used by the tick. This must be identical to
// PortYield() from the call to TaskSwitchContext() onwards.
// The only difference from PortYield() is the tick is incremented as the 
// call comes from the tick ISR.
void PortYieldFromTick(void) __attribute__((naked));
void PortYieldFromTick(void)
{
    PortSaveContext();
    TaskIncrementTick();
    TaskSwitchContext();
    PortRestoreContext();

    asm volatile ("ret");
}

// 
// 分周: 64
// Timerトップ値: 255
//
// クロック周波数ごとのTimer割り込み周波数:
//  クロック周波数: 16Mhz
//   Timer割り込み周波数: 16Mhz / (256 * 64) = 976.5625 Hz
//
// Setup timer 0 compare match A to generate a tick interrupt.
static void SetupTimerInterrupt( void )
{
    // On the ATmega168, timer 0 is also used for fast hardware pwm
    // (using phase-correct PWM would mean that timer 0 overflowed half as often
    // resulting in different mills() behavior on ATmega8 and ATmage168)
#if defined(TCCR0A) && defined(WGM01)
    sbi(TCCR0A, WGM01);
    sbi(TCCR0A, WGM00);
#endif

    // set timer 0 prescale factor to 64
#if defined(__AVR_ATmega128__)
    // CPU spwcific: different values for the ATmega128
    sbi(TCCR0, CS02);

#elif defined(TCCR0) && defined(CS01) && defined(CS00)
    // This combination is for the standard atmega8
    sbi(TCCR0, CS01);
    sbi(TCCR0, CS00);

#elif defined(TCCR0B) && defined(CS01) && defined(CS00)
    // This combination is for the standard 168/328/1280/2560
    sbi(TCCR0B, CS01);
    sbi(TCCR0B, CS00);

#elif defined(TCCR0A) && defined(CS01) && defined(CS00)
    // This combination is for the standard __AVR_ATmega645__ series
    sbi(TCCR0A, CS01);
    sbi(TCCR0A, CS00);
#else
    #error Timer 0 prescale factor 64 not set correctry
#endif

    // enable timer 0 overflow interrupt
#if defined(TIMSK) && defined(TOIE0)
    sbi(TIMSK, TOIE0);
#elif defined(TIMSK0) && defined(TOIE0)
    sbi(TIMSK0, TOIE0);
#else
    #error Timer 0 overflow interrupt not set correctly
#endif
}

#if CONFIG_USE_PREEMPTION == 1
// Tick ISR(Interrupt Service Routine) for preemptive scheduler. We can use a naked attribute as
// the context is saved at the start of PortYieldFromTick(). The tick count
// is incremented after the context is saved.
    #if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84)
        void TIM0_OVF_vect(void) __attribute__((signal, __INTR_ATTRS, naked));
        void TIM0_OVF_vect(void)
    #else
        void TIMER0_OVF_vect(void) __attribute__((signal, __INTR_ATTRS, naked));
        void TIMER0_OVF_vect(void)
    #endif
    {
        PortYieldFromTick();
        asm volatile("reti");
    }

#else
// Tick ISR(Interrupt Service Routine) for the cooperative scheduler. All this does is increment
// the tick count. We do't need to switch context, this can only be done by manual calls to TaskYield()
    #if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84)
        void TIM0_OVF_vect(void) __attribute__((signal, __INTR_ATTRS));
        void TIM0_OVF_vext(void)
    #else
        void TIMER0_OVF_vect(void) __attribute__((signal, __INTR_ATTRS));
        void TIMER0_OVF_vect(void)
    #endif
    {
        TaskIncrementTick();
    }
#endif