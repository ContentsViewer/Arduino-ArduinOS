/*
// ArduinOS 
//
// ArduinOSとは, Arduinoで使用できるOSです. 
// このOSを使用することで, マルチタスクによるプログラミングが可能になります.
// このOSの原型はFreeRTOSです.
// 
// システムフック:
//  ArduinOSには, システムフックが用意されています.
//  システムフックとは, システムのある特定のイベントで利用者が独自の処理を追加できるようにする仕組みです.
//  各システムフック名, 呼び出される関数名, 設定項目は以下の通り
//   
//   名前               関数                            設定項目                        イベント箇所
//   IdleHook           ApplicationIdleHook()           CONFIG_USE_IDLE_HOOK            IdleTask実行時
//   TickHook           ApplicationTickHook()           CONFIG_USE_TICK_HOOK            システム時間インクリメント時
//   MallocFailedHook   ApplicationMallocFailedHook()   CONFIG_USE_MALLOC_FAILED_HOOK   PortMalloc()関数によるメモリ確保失敗時. malloc()関数ではない.
//   StackOverflowHook  ApplicationStackOverflowHook()  CONFIG_CHECK_FOR_STACK_OVERFLOW スタックオバーフロー検知時
/
// 設定:
//  ArduinOSを正常に稼働させるためにハードウェアごとの設定が必要です.
//  ArduinOS.h と同階層にArduinOSConfig.hを作成し, そのヘッダファイル内で各種設定を行って下さい.
//  設定方法としては, あらかじめ定められているマクロに値を設定することです.
//  例えば, OSによるタスク自動遷移を行いたい場合はマクロ名"CONFIG_USE_PREEMPTION"を"1"に設定します.
//  
//  設定項目一覧:
//   必ず設定しなければならないもの
//    CONFIG_USE_PREEMPTION:
//    CONFIG_USE_IDLE_HOOK:
//    CONFIG_USE_TICK_HOOK:
//    CONFIG_USE_16_BIT_TICKS:
//    CONFIG_MINIMAL_STACK_SIZE:
//    CONFIG_TOTAL_HEAP_SIZE:
//    CONFIG_TICK_RATE_HZ:
//    INCLUDE_TASK_DELETE
//    INCLUDE_TASK_SUSPEND
//    INCLUDE_TASK_DELAY_UNTIL
//    INCLUDE_TASK_DELAY
//
//   カスタム設定一覧:
//    カスタム設定は明示的に設定する必要はない.
//    もし設定されなかった場合は自動でデフォルト値が設定される.
//
//    マクロ名(デフォルト値)
//    CONFIG_USE_MALLOC_FAILED_HOOK(0)
//    CONFIG_USE_MUTEXES(0)
//    CONFIG_MAX_PRIORITIES(3)
//    CONFIG_IDLE_SHOULD_YIELD(1)
//    CONFIG_MAX_TASK_NAME_LEN(16)
//    CONFIG_CHECK_FOR_STACK_OVERFLOW(1)
//    INCLUDE_TASK_GET_STATE(0)
//    INCLUDE_TASK_GET_SCHEDULER_STATE(0)
//    INCLUDE_TASK_GET_CURRENT_TASK_HANDLE(0)
//    INCLUDE_TASK_RESUME_FROM_ISR(1)
//    
//
// 対応マイコン:
//  ATmega328
//  ATmega2560
//
// デフォルトArduinoコードに変更を加えたファイル名一覧:
//  Arduino.h
//  main.cpp
//  wiring.c
//  WString.h
//  new.cpp
//  
//
// ハードウェア依存ファイル一覧:
//  Port.c
//  Portable.h
//  PortMacro.h
//
//  AVRマイコン以外のマイコンを使用する際, 特に,
//  ハードウェア構造がAVRマイコンと異なる場合は,
//  これらファイルを変更する必要がある.
//
//  ハードウェアごとに異なる特徴としては以下が挙げられる
//   CPUデータバスの構成幅(8bit, 16bitなど)
//   Timer設定の方法
//   レジスタの数
//   
//   
//
// 更新履歴:
//  2017/6/22:
//   初期版完成
//
//  2017/8/24:
//   メモリの断片化が起こらないようにHeapを変更.
//
//  2017/8/29:
//   スタックオバーフロー検知機能追加
//
//  2017/10/20:
//   malloc関数との競合を解消.
//   arduino標準delay()関数を本来の機能に戻す.
//   delay()関数とTaskDelay()を区別化
//   Acquire()のタイムアウト時間をmsで指定できるように変更
//   MegaOSからArduinOSへ名称変更
//   mainLoopTaskの優先度, およびスタックサイズをスケッチ内で変更できるようにした
//
//  2017/11/1:
//   Includeガードにかかりやすい問題を修正
*/

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
#ifndef ARDUINOS_H
#define ARDUINOS_H

//Include the generic headers required for the ArduinOS port being used
#include <stddef.h>

#include <stdint.h>

//Basic ArduinOS definitions
#include "ProjDefs.h"

// Application specific configuration options.
#include "ArduinOSConfig.h"

// Definitions specific to the port being used.
#include "Portable.h"


// Defines the prototype to which the application task hook function must
// conform. 
typedef PortBaseType(*TaskHookCode)(void *);

// --------------------------------------------------------------------------
// Check all the required application specific macros have been defined.
// These macro are application specific and (as downloaded) are defined
// within ArduinOSConfig.h
// --------------------------------------------------------------------------

// ---　必ず定義しなければならない ---------------------------------------
// --- Must define -------------------------------------------------------

//
// OSによる自動タスク遷移を行うか
//
#ifndef CONFIG_USE_PREEMPTION
    #error Missing definition: CONFIG_USE_PREEMPTION should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// IdleTask内で実行される関数(ApplicationIdleHook())を有効にするか
//
#ifndef CONFIG_USE_IDLE_HOOK
    #error Missing definition: CONFIG_USE_IDLE_HOOK should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// システム時間がインクリメントされるごとに実行される
// 関数(ApplicationTickHook())を有効にするか
//
#ifndef CONFIG_USE_TICK_HOOK
    #error Missing definition: CONFIG_USE_TICK_HOOK should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// システム時間の最大値を2Byte分(0xffff)にするか.
// それ以外の場合は, 4Byte分(0xffffffff)になります.
//
#ifndef CONFIG_USE_16_BIT_TICKS
    #error Missing definition: CONFIG_USE_16_BIT_TICKS should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// 各タスクで使用されるスタックの最小値
//
#ifndef CONFIG_MINIMAL_STACK_SIZE
    #error Missing definition: CONFIG_MINIMAL_STACK_SIZE should be defined in ArduinOSConfig.h.
#endif

//
// Kernel内部のヒープ領域
//
#ifndef CONFIG_TOTAL_HEAP_SIZE
    #error Missing definition: CONFIG_TOTAL_HEAP_SIZE should be defined in ArduinOSConfig.h.
#endif

//
// システム時間更新周波数
//
#ifndef CONFIG_TICK_RATE_HZ
    #error Missing definition: CONFIG_TICK_RATE_HZ should be defined in ArduinOSConfig.h.
#endif

//
// Task削除を有効にするか
//
#ifndef INCLUDE_TASK_DELETE
    #error Missing definition: INCLUDE_TASK_DELETE should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// Task一時停止を有効にするか
//
#ifndef INCLUDE_TASK_SUSPEND
    #error Missing definition: INCLUDE_TASK_SUSPEND should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// TaskDelayUntilを有効にするか
//
#ifndef INCLUDE_TASK_DELAY_UNTIL
    #error Missing definition: INCLUDE_TASK_DELAY_UNTIL should be defined in ArduinOSConfig.h as either 1 or 0.
#endif

//
// TaskDelayを有効にするか
//
#ifndef INCLUDE_TASK_DELAY
    #error Missing definition: INCLUDE_TASK_DELAY should be defined in ArduinOSConfig.h as either 1 or 0.
#endif
// Must define end ------------------------------------------------------

// --- カスタム設定 ------------------------------------------------------------------
//
//   明確に設定しなくてよい. 設定しなかった場合は, 自動でデフォルト値が設定される.

// --- Config系 --------------------------------------------------------
#ifndef CONFIG_USE_MALLOC_FAILED_HOOK
    #define CONFIG_USE_MALLOC_FAILED_HOOK 0
#endif

/*
#ifndef CONFIG_USE_TIMERS
    #define CONFIG_USE_TIMERS 0
#endif
*/

#ifndef CONFIG_USE_MUTEXES
    #define CONFIG_USE_MUTEXES 0
#endif

#ifndef CONFIG_MAX_PRIORITIES
    #define CONFIG_MAX_PRIORITIES  ((unsigned PortBaseType)3)
#endif

#ifndef CONFIG_IDLE_SHOULD_YIELD
    #define CONFIG_IDLE_SHOULD_YIELD 1
#endif

#ifndef CONFIG_MAX_TASK_NAME_LEN
    #define CONFIG_MAX_TASK_NAME_LEN 16
#endif

#if (CONFIG_MAX_TASK_NAME_LEN < 1)
    #error CONFIG_MAX_TASK_NAME_LEN must be set to a minimum of 1 in MagaOSConfig.h.
#endif

#ifndef CONFIG_CHECK_FOR_STACK_OVERFLOW
    // スタックオバーフローを検知したとき, ApplicationStackOverflowHook()を呼び出します.
    //  
    // 0: スタックオバーフローを検知しない
    // 1: 検知方法1を使用. 詳しくはStackMacros.hをご覧ください.
    // 2以上: 検知方法1,2 を使用. 詳しくはStackMacros.hをご覧ください.
    // 
    // 検知方法2は1より厳密になります.
    #define CONFIG_CHECK_FOR_STACK_OVERFLOW 0
#endif

// End Config系 ---------------------------------------------------

// --- Include系 -----------------------------------------------------

#ifndef INCLUDE_TASK_GET_STATE
    #define INCLUDE_TASK_GET_STATE 0
#endif

#ifndef INCLUDE_TASK_GET_SCHEDULER_STATE
    #define INCLUDE_TASK_GET_SCHEDULER_STATE 0
#endif

#ifndef INCLUDE_TASK_GET_CURRENT_TASK_HANDLE
    #define INCLUDE_TASK_GET_CURRENT_TASK_HANDLE 0
#endif

#ifndef INCLUDE_TASK_RESUME_FROM_ISR
    #define INCLUDE_TASK_RESUME_FROM_ISR 1
#endif
// End Include系 --------------------------------------------
// End カスタム設定  --------------------------------------

#ifndef PortPointerSizeType
    #define PortPointerSizeType unsigned long
#endif

#ifndef PortYieldWithinAPI
    #define PortYieldWithinAPI() PortYield()
#endif

//アライメントされたメモリを確保する
#ifndef PortMallocAligned
    #define PortMallocAligned(x, stackBuffer) (((stackBuffer) == NULL) ? (PortMalloc((x))) : (stackBuffer))
#endif

// アライメントされたメモリを開放する
#ifndef PortFreeAligned
    #define PortFreeAligned(blockToFree) PortFree(blockToFree)
#endif



// ---------------------------------------------------------------
// -------------------------------------------------------------------------
// TRACE MACRO
// -------------------------------------------------------------------------

// The following event macros are embeded in the kernel API calls.
#ifndef TraceTaskSwitchedIn
    // Called after a task has been selected to run.
    // currentTCB holds a pointer to the task control block of the selected task.
    #define TracetaskSwitchedIn()
#endif

#ifndef TraceTaskSwitchedOut
    // Called before a task has been selected to run.
    // currentTask holds a pointer to the task control block of the task being switched out.
    #define TracetaskSwitchedOut()
#endif

#ifndef TraceTaskPriorityInherit
    // Called when a task attempts to take a mutex that is already held by a
    // lower priority task. tcbOfMutexHolder is a pointer to the TCB of the task
    // that holds the mutex. inheritedPriority is the priority the mutex holder
    // will inherit (the priority of the task that is attempting to obtain the
    // muted.
    #define TraceTaskPriorityInherit(tcbOfMutexHolder, inheritedPriority)
#endif

#ifndef TarceTaskPriorityDisinherit
    // Called when a task release a mutex, the holding of which had resulted in
    // the task inheriting the priority of a higher priority task.
    // tcbOfMutexHolder is a pointer to the TCB of the task that is releasing the
    // mutex. originalPriority us the task's configured (base) priority.
    #define TraceTaskPriorityDisinherit(tcbOfMutexHoldrer, originalPriority)
#endif

#ifndef TraceTaskCreate
    #define TraceTaskCreate(newTCB)
#endif

#ifndef TraceTaskCreateFailed
    #define TraceTaskCreateFailed()
#endif

#ifndef TraceTaskDelete
    #define TraceTaskDelete(taskToDelete)
#endif

#ifndef TraceTaskDelayUntil
    #define TraceTaskDelayUntil()
#endif

#ifndef TraceTaskDelay
    #define TraceTaskDelay()
#endif

#ifndef TraceTaskSuspend
    #define TraceTaskSuspend(taskToSuspend)
#endif

#ifndef TraceTaskResume
    #define TraceTaskResume(taskToResume)
#endif

#ifndef TraceTaskResumeFromISR
    #define TraceTaskResumeFromISR(taskToResume)
#endif

#ifndef TraceTaskIncrementTick
    #define TraceTaskIncrementTick(tickCount)
#endif

// --- Queue 関係 --------------------------------------------
#ifndef TraceQueueCreate
    #define TraceQueueCreate(newQueue)
#endif

#ifndef TraceQueueCreateFailed
    #define TraceQueueCreateFailed(queueType)
#endif

#ifndef TraceCreateMutex
    #define TraceCreateMutex(newQueue)
#endif

#ifndef TraceCreateMutexFailed
    #define TraceCreateMutexFailed()
#endif

#ifndef TraceQueueDelete
    #define TraceQueueDelete(queue)
#endif

#ifndef TraceQueuePeek
    #define TraceQueuePeek(queue)
#endif

// --- Receive 関係 ------
#ifndef TraceQueueReceive
    #define TraceQueueReceive(queue)
#endif

#ifndef TraceQueueReceiveFailed
    #define TraceQueueReceiveFailed(queue)
#endif

#ifndef TraceQueueReceiveFromISR
    #define TraceQueueReceiveFromISR(queue)
#endif

#ifndef TraceQueueReceiveFromISRFailed
    #define TraceQueueReceiveFromISRFailed(queue)
#endif

// ----------------------
// --- Send 関係 --------
#ifndef TraceQueueSend
    #define TraceQueueSend(queue)
#endif

#ifndef TraceQueueSendFailed
    #define TraceQueueSendFailed(queue)
#endif

#ifndef TraceQueueSendFromISR
    #define TraceQueueSendFromISR(queue)
#endif

#ifndef TraceQueueSendFromISRFailed
    #define TraceQueueSendFromISRFailed(queue)
#endif

// ---------------------

#ifndef TraceBlockingOnQueueReceive
    // Task is about to block because it cannot read from a
    // queue/mutex/semaphore.  queue is a pointer to the queue/mutex/semaphore
    // upon which the read was attempted.  currentTCB points to the TCB of the
    // task that attempted the read. 
    #define TraceBlockingOnQueueReceive(queue)
#endif

#ifndef TraceBlockingOnQueueSend
    // Task is about to block because it cannot write to a
    // queue/mutex/semaphore.  queue is a pointer to the queue/mutex/semaphore
    //upon which the write was attempted.  currentTCB points to the TCB of the
    //task that attempted the write. 
    #define TraceBlockingOnQueueSend(queue)
#endif

// End Queue 関係 -------------------
// ---------------------------------------------------------------
#include "Task.h"
#include "Portable.h"

#include "Semaphore.h"

// ---------------------------------------------------------------
// アプリケーションとOS間の中間関数
//

// 優先度マクロ
#define LOW_PRIORITY (IDLE_TASK_PRIORITY)
#define NORMAL_PRIORITY (IDLE_TASK_PRIORITY + 1)
#define HIGH_PRIORITY (IDLE_TASK_PRIORITY + 2)


//
// mainLoopのスタックサイズおよび優先度
// これらは, main.cpp内で定義されている. 
// mainLoopのスタックサイズ, 優先度はスケッチ内で変更できるようにする.
//
extern unsigned PortBaseType mainLoopPriority;
extern unsigned short mainLoopStackSize;

//
// mainLoopの優先度を設定します.
// これは, setup()関数内でのみ有効です.
//
#define InitMainLoopPriority(priority)                      \
    mainLoopPriority = (unsigned PortBaseType)(priority);


//
// mainLoopのスタックサイズを設定します.
// これは, setup()関数内でのみ有効です.
//
#define InitMainLoopStackSize(stackSize)                    \
    mainLoopStackSize = (unsigned short)(stackSize);
    

/*
//
// タスクコードを書く前の宣言マクロ
// このマクロはタスクハンドル, タスクコード関数など, Kernelに渡すための
// 関数などを作成します.
//
// このマクロを書いてから, 実際に行いたいタスクコードを書きます.
// この宣言子の下にあるブロックがタスクコードとなります.
//
// @param name:
//  タスク名
//
// Example usage:

// タスク宣言
DeclareTaskLoop(taskA);

// TaskCode宣言
TaskLoop(taskA)
{
    // ここに, 処理を書く.
    // ここに書かれたコードは繰り返されます.
}
*/
#define TaskLoop(name)                                                              \
    static inline void name##_Function();                                           \
    /*void name##_Task(void *);*/                                                   \
    TaskHandle name;                                                                \
    void name##_Task(void *parameters)                                              \
    {                                                                               \
        for (;;)                                                                    \
            name##_Function();                                                      \
    }                                                                               \
    static inline void name##_Function()    

/*
//
// タスクループ宣言マクロ
// タスクハンドルの宣言, タスクコード関数の宣言を行います
//
// TaskLoop()前に宣言する必要があります.
//
// @param name:
//  タスク名
//
// Example usage:

// タスク宣言
DeclareTaskLoop(taskA);

// TaskCode宣言
TaskLoop(taskA)
{
    // ここに, 処理を書く.
    // ここに書かれたコードは繰り返されます.
}
*/
#define DeclareTaskLoop(name)                                                       \
    extern TaskHandle name;                                                         \
    /*void name##_Task(void *)*/

//
// タスクを作成するマクロ
// このマクロは, KernelにTaskを登録します.
// タスクを作成するためにTaskLoop()でタスクコードを作成する必要があります.
//
// @param name:
//  タスク名
//
// @param priority:
//  タスク優先度
//  タスク優先度の種類は合計三つです.
//  次のマクロを使用してください. (LOW_PRIORITY, NORMAL_PRIORITY, HIGH_PRIORITY)
//  計算リソースの量の点で, 優先度が高いタスクは低いタスク以上です.
//
#define CreateTaskLoop(name, priority)                                                                      \
                                                                                                            \
    TaskCreate(name##_Task, (signed PortChar*) #name, CONFIG_MINIMAL_STACK_SIZE, NULL, priority, &name);    \


//
// タスクを作成します.
// このマクロはスタックサイズを設定することができます.
// タスクを作成するためにTaskLoop()でタスクコードを作成する必要があります.
//
// @param name:
//  タスク名
//
// @param priority:
//  タスク優先度
//  タスク優先度の種類は合計三つです.
//  次のマクロを使用してください. (LOW_PRIORITY, NORMAL_PRIORITY, HIGH_PRIORITY)
//  計算リソースの量の点で, 優先度が高いタスクは低いタスク以上です.
//
// @param stackSize:
//  設定するスタックのサイズ. 
//  スタックには次のものが詰まれます.
//   ローカル変数
//   関数を呼ぶときに保存されるデータ(リターンアドレス等)
//
//  すなわち, スタックサイズは小さすぎると多くのローカル変数や
//  多くの関数を呼べなくなります.
//
#define CreateTaskLoopWithStackSize(name, priority, stackSize)                              \
{                                                                                           \
    TaskCreate(name##_Task, (signed PortChar *) #name, stackSize, NULL, priority, &name);   \
}

//
// このコード(Suspend)が書かれているTaskを一時停止します.
// 一時停止されたタスクは, ResumeTask()を実行するまで再開されません.
// 
#define Suspend() TaskSuspend(NULL)

//
// 指定されたTaskを一時停止状態にします.
// 一時停止されたタスクは, ResumeTask()を実行するまで再開されません.
// 
// @param name:
//  一時停止をしたいTask名
//
#define SuspendTask(name)                                                                   \
    if((name) != NULL)                                                                      \
    {                                                                                       \
        TaskSuspend(name);                                                                  \
    }

//
// すべてのタスクを一時停止状態にします.
// ResumeAll()を実行するまで, これらタスクは再開せれません.
//
#define SuspendAll() TaskSuspendAll()

//
// 一時停止されたタスクを再開します.
//
// @param name:
//  再開したいタスク名
//
#define ResumeTask(name)                                                                    \
    if((name) != NULL)                                                                      \
    {                                                                                       \
        TaskResume(name);                                                                   \
    }

//
// SuspendAll()によって, 一時停止されたすべてのタスクを再開します.
//
#define ResumeAll() TaskResumeAll()

//
// このコードが呼ばれた時点で, ほかのタスクに処理を移します.
// 処理の再開位置はこのコード以降からです.
//
#define Yield() TaskYield()

//
// このコードが書かれているタスクを削除します.
//
#define Delete() TaskDelete(NULL)

//
// 指定されたタスクを削除します.
//
// @param name:
//  削除したいタスク名
//
#define DeleteTask(name)                                                                    \
    if((name) != NULL)                                                                      \
    {                                                                                       \
        TaskDelete(name);                                                                   \
    }

/*
// クリティカルセクションに入ることを宣言します.
// あるタスクがクリティカルセクションに入っている間は,
// ほかのタスクは実行されなくなります.
// これにより単一リソースに対する衝突を防ぎます.
//
// 警告:
//  クリティカルセクションに入る時間は短くすべきです.
//  クリティカルセクション中では, Kernelの割り込みも停止されており,
//  システム時間も更新されません. また, ほかタスクの処理も行われなくなります.
// 
// Example usage:

// タスク宣言
DeclareTaskLoop(taskA);

// TaskCode宣言
TaskLoop(taskA)
{
    // ...

    // クリティカルセクションに入る
    EnterCritical();
    {
        // ここのブロック内では, OSによる割り込みは行われない.
        // ...

        // クリティカルセクションから出る.
        ExitCritical();
    }
}
*/
#define EnterCritical() PortEnterCritical()

/*
// クリティカルセクションに出ることを宣言します.
// あるタスクがクリティカルセクションに入っている間は,
// ほかのタスクは実行されなくなります.
// これにより単一リソースに対する衝突を防ぎます.
//
// 警告:
//  クリティカルセクションに入る時間は短くすべきです.
//  クリティカルセクション中では, Kernelの割り込みも停止されており,
//  システム時間も更新されません. また, ほかタスクの処理も行われなくなります.
// 
// Example usage:

// タスク宣言
DeclareTaskLoop(taskA);

// TaskCode宣言
TaskLoop(taskA)
{
    // ...

    // クリティカルセクションに入る
    EnterCritical();
    {
        // ここのブロック内では, OSによる割り込みは行われない.
        // ...

        // クリティカルセクションから出る.
        ExitCritical();
    }
}
*/
#define ExitCritical() PortExitCritical()

//
// 指定時間の間, タスクを停止します.
// 
// @param ms:
//  待機時間(ms)
//  
#define DelayWithBlocked(ms) TaskDelay((ms) / PORT_TICK_RATE_MS)

#define GetTickCount() TaskGetTickCount()

/*
//
// @param previousWakeTime:
//  タスクが待機状態から解放されたときの時間変数のポインタ.
//  使用前に必ず現在時刻(tick)で初期化する必要があります. GetTickCount()を使用してください.
//  のちのこの値は, 自動でDelayUntilWithBlocked()で更新されます.
//
// @param frequency:
//  サイクル周期(ms).
//  タスクが次に待機状態を抜ける時間は, previousWakeTime(tick) + frequency(ms) / PORT_TICK_RATE_MS(ms / tick) です.
//
// Example usage :

TaskLoop(taskA)
{
    unsigned long lastWakeTime;

    // 1000msごとの周期
    const unsigned long frequency = 1000;

    // lastWakeTime変数を現在時刻で初期化
    lastwakeTime = GetTickCount();
    for (;;)
    {
        // 次のサイクルまで待機
        DelayUntilWithBlocked(&lastWakeTime, frequency);

        // 何かルーチン処理
        // ...
    }
}
*/
#define DelayUntilWithBlocked(previousWakeTime, frequency)              \
    TaskDelayUntil((previousWakeTime), (frequency) / PORT_TICK_RATE_MS)
// ------------------------------------------------------

// --- Semaphore, Mutex 関係 -------------------------------------------
//
//


/*
// バイナリセマフォを作成します.
// このタイプのセマフォは, 純粋なタスク間の同期に用いられます.
// 動機が必要な例として, 複数のタスクが一つのリソースにアクセスするときなどが挙げられるでしょう.
// あるタスクが, セマフォを獲得している際, 他のタスクはこのセマフォを獲得できません.
// 獲得しているタスクがセマフォを開放したとき, 他のタスクはこのセマフォを獲得できます.
//
// @param semahore:
//  作成されたセマフォへのハンドル
//
// Example usage :

SemaphoreHandle semaphore;

TaskLoop(taskA)
{
    // セマフォはCreateBinarySemaphore()を呼ぶまで使用できません.
    CreateBinarySemaphore(semaphore);

    if (semaphore != NULL)
    {
        // セマフォの作成に成功したとき.
        // ...
    }
}
*/
#define CreateBinarySemaphore(semaphore)                  \
    SemaphoreCreateBinary(semaphore)

/*
// ミューテックスを作成します.
// このタイプのセマフォは, バイナリセマフォとほとんど同じですが,
// 優先度継承を行う点が異なります.
// 優先度継承とは, セマフォ獲得による優先度の逆転の問題を解決します.
//
// @param semahore:
//  作成されたセマフォへのハンドル
//
// Example usage :

SemaphoreHandle semaphore;

TaskLoop(taskA)
{
    // セマフォはCreateMutex()を呼ぶまで使用できません.
    CreateMutex(semaphore);

    if (semaphore != NULL)
    {
        // セマフォの作成に成功したとき.
        // ...
    }
}
*/
#define CreateMutex(semaphore)                            \
    (semaphore) = SemaphoreCreateMutex()

/*
// セマフォを獲得します.
// そのセマフォは必ずCreate関数であらかじめ作成する必要があります.
// 
// @param semaphore:
//  作成したセマフォ
//
// @param blockTime:
//  獲得するまでの最大待機時間(ms)
//  この時間を超えても, セマフォを獲得できない場合,
//  獲得処理を停止します.
//
// Example usage:


SemaphoreHandle semaphore;

TaskLoop(taskA)
{
    // セマフォはCreateBinarySemaphore()を呼ぶまで使用できません.
    CreateBinarySemaphore(semaphore);

    if (semaphore != NULL)
    {
        // セマフォの作成に成功したとき.
        // ...

        // セマフォを獲得するのに, 100ms待機する
        if(Acuire(semaphore, 100)){
            // 獲得できた時
            
            // ...

            // 共有しているリソースへの処理を終えたとき,
            // セマフォの開放
            Relese(semaphore);
        else{
            // 獲得できなかったとき
            // 共有しているリソースへのアクセス権が得られなかった.
        }
    }
}

*/
#define Acquire(semaphore, blockTime)                     \
    (SemahoreTake((semaphore), ((PortTickType)(blockTime)) / PORT_TICK_RATE_MS) == PD_TRUE)


/*
// セマフォを解放します.
// そのセマフォは必ずCreate関数であらかじめ作成する必要があります.
// 
// @param semaphore:
//  作成したセマフォ
//
// Example usage:


SemaphoreHandle semaphore;

TaskLoop(taskA)
{
    // セマフォはCreateBinarySemaphore()を呼ぶまで使用できません.
    CreateBinarySemaphore(semaphore);

    if (semaphore != NULL)
    {
        // セマフォの作成に成功したとき.
        // ...

        // セマフォを獲得するのに, 100ms待機する
        if(Acuire(semaphore, 100)){
            // 獲得できた時
            
            // ...

            // 共有しているリソースへの処理を終えたとき,
            // セマフォの開放
            Relese(semaphore);
        else{
            // 獲得できなかったとき
            // 共有しているリソースへのアクセス権が得られなかった.
        }
    }
}

*/
#define Release(semaphore)                                \
    (SemaphoreGive((semaphore)) == PD_TRUE)

// End Semaphore, Mutex 関係 -----------------
#endif