
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
// ArduinOSのタスク管理部分です.
// Taskの作成, 切り替え, 削除などを行います. 
*/


#ifndef TASK_H
#define TASK_H


#ifndef ARDUINOS_H
#error "include ArduinOS.h must appear in source files before include Task.h"
#endif

#include "Portable.h"
#include "List.h"


#ifdef __cplusplus
extern "C" {
#endif
    // ----------------------------------------------------------------------
    // MACROS AND DEFINITION
    // ----------------------------------------------------------------------

    // タスクを参照するための型
    // アプリケーション側ではこれを使用してどのタスクかをOSに伝える
    // Type by which tasks are referenced. For example, a call to TaskCreate
    // returns (via a pointer parameter) an TaskHandle variable that can then
    // be used as a parameter to TaskDelete to delete the task.
    typedef void *TaskHandle;

    // Used internally only.
    typedef struct
    {
        PortBaseType overflowCount;
        PortTickType timeOnEntering;
    }TimeOutType;

    //Task states returned by TaskGetState
    typedef enum
    {
        // A task is querying the state og itself, so must be running.
        Running = 0,

        // The task being queryied is in a read or pending ready list.
        Ready,

        // The task being queried is in the Blocked state.
        Blocked,

        // The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out.
        Suspended,

        // The task being queried has been deleted, but its TCB has not yet been freed.
        Deleted
    }TaskState;

    // possible retrun values for TaskConfirmSleepStatus().
    enum SleepModeStatus
    {
        AbortSleep = 0,
        StandardSleep,
        NoTasksWaitingTimeout
    };

    // IdleTaskの優先度, 変更すべきではない.
    // Defines the priority used by the idle task. This must not be modified.
#define IDLE_TASK_PRIORITY ((unsigned PortBaseType) 0U)

// 強制的にタスクを切り替える
// Macro for forcing a context switch
#define TaskYield() PortYield()

// Macro to mark the start of a critical code region. Preemptive context
// switches cannot occur when in a critical region.
//
// NOTE: This may alter the stack (depending on the portable implementation)
// so must be used with care!
#define TaskEnterCritical() PortEnterCritical()

// Macro to mark the end of a critical code region. Preemptive context
// switches cannot occur when in a critical region.
//
// NOTE: This may alter the stack (depending on the portable implementation)
// so must be used with care!
#define TaskExitCritical()  PortExitCritical()

// Macro to disable all maskable interrupts.
#define TaskDisableInterrupts() PortDisableInterrupts()

// Macro to enable microcontroller interrupts.
#define TaskEnableInterrupts() PortEnableInterrupts()

// Definitions returned by TaskGetSchedulerState()
#define TASK_SCHEDULER_NOT_STARTED 0
#define TASK_SCHEDULER_RUNNING 1
#define TASK_SCHEDULER_SUSPENDED 2


//-------------------------------------------------------------------
// TASK CREATION API
//-------------------------------------------------------------------

/*
// Create a new task and it to the list of tasks that are ready to run.
//
// TaskCreate() can only be used to create a task that has unrestricted
// access to the entire microcontroller memory map. Systems that include MPU
// suport can alternatively create an MPU constrained task using TaskCreateRestricted()
//
// @param taskCode:
//  Pointer to the task entry function. Tasks must be implemented to never return (i.e. continuous loop).
//
// @param name:
//  A descriptive name for the task. This is mainly used to
// facilitate debuggering. Max length defined by TASK_MAX_TASK_NAME_LEN - default is 16
//
// @param stackDepth:
//  The size of the task stack specified as the number of variables the stack
//  can hold - not the number of bytes. For example, if the stack is 16 bits wide
//  and stackDepth us defined as 100, 200 bytes will be allocaed for stack storage.
//
// @param parameters:
//  Pointer that will be used as the paramerter for the task being created.
//
// @param priority:
//  The priority at which the task should run.
//
// @param createdTask:
//  Used to pass back a handle by which the created task can be referenced.
//
// @return PD_PASS:
//  if the task was successfully created and added to a ready list,
//  otherwise an error code defined in the file "ProjDefs.h"
//
// TaskHandle の詳しい説明:
//  TaskHandleの型は (void *) です.
//  TCBのメモリアドレスを保存するものです.
//  Create関数に渡すとき, TCBのアドレスを保存するメモリのアドレスを渡します.
//  つまり, 関数の引数には &handle と書きます. 渡された関数ではvoid ** となります.
//  これで, Create関数はhandleにTCBのメモリアドレスを格納することができます.
// 
// Example usage:

// Task to be created.

void TaskCodeFunction(void *paramters)
{
    for (;;)
    {
        // Task code goes here
    }
}

// Function that creates a task
void OtherFunction(void)
{
    static unsigned char parameterToPass;
    TaskHandle handle;

    // Create the task, storeing the handle. Note that the passed parameter parameterTopass
    // must exist for the lifeteime oh the task, so in this case is declared static.
    // If it was just an automatic stack variable it might no longer exist, or at least have been corrupted,
    // by the time the new task attempts to access it.
    TaskCreate(TaskCodeFunction, "NAME", STACK_SIZE, &parameterToPass, IDLE_TASK_PRIORITY, &handle);

    // Use the handle to delete the task.
    TaskDelete(handle);
}
*/
#define TaskCreate(taskCode, name, stackDepth, parameters, priority, createdTask) \
    TaskGenericCreate((taskCode), (name), (stackDepth), (parameters), (priority), (createdTask), (NULL))

/*
//
// INCLUDE_TASK_DELETE must be defined as 1 for this function to be available.
// See the configuration section for more information.
//
// Remove a task from RTOS real time kernels management. The task being
// deleted will be removed from all ready, blocked, suspended, and event lists.
//
// NOTE:
//  The idle task is responsible for freeing the kernel allocated
//  memory from tasks that have been deleted. It is therfore important taht
//  the idle task is not starved of microcontroller processing time if
//  your application makes any calls to TaskDelete(). Memory allocated by the
//  task code is not automatically freed, and should be freed before the task
//  is deleted.
//
//  Idleタスク内で初めてタスクがメモリ上から削除される.
//
// @param taskToDelete:
//  The handle of the task to be deleted. Passing NULL will
//  cause the calling task to be deleted.
//
// Exapmle usage:

void OtherFunction(void)
{
    TaskHandle handle;

    // Create the task, storeing the handle.
    TaskCreate(TaskCodeFunction, "NAME", STACK_SIZE, NULL, IDLE_TASK_PRIORITY, &handle);

    // Use the handle to delete the task.
    TaskDelete(handle);
}
*/
    void TaskDelete(TaskHandle taskToDelete);

    // END TASK CREATION API -----------------------------------

    //-------------------------------------------------------------------
    // TASK CONTROL API
    //-------------------------------------------------------------------

    /*
    //
    // Delay a task for a given number of ticks. The actual time that the
    // task remains blocked depends on the tick rate. The constant
    // PORT_TICK_RATE_MS can be used to caluculate real time from the tick rate
    // - with the resoulution of one tick period.
    //
    // INCLUDE_TASK_DELAY must be defined as 1 for this function to be available.
    // See the configuration section for more information.
    //
    // TaskDelay() specifies a time at which the task wishes to unblock relative to
    // the time at which TaskDelay() is called. For example, specifying a block
    // period of 100 ticks will cause the task to unblock 100 ticks after
    // TaskDelay() is called. TaskDelay() does not therfore provide a good method
    // of controlling the frequency of a cyclical task as the path taken throgh the
    // code, as well as other task and interrupt activity, will effect the frequency
    // at which TaskDelay() gets called and therfore the time at which the task
    // next executes. See TaskDelayUntil() for an alternative API function designed
    // to faciliate fixed frequency execution. It does this by specifying an
    // absolute time (rather than a relatibe time) at which the calling task should
    // unblock.
    //
    // NOTE:
    //  Delay, DelayUntilの違い:
    //   あるタスク内で一定間隔で処理を実行したい場合はDelayよりもDelayUntilの方が向いている.
    //   Delayの挙動は以下の通り.
    //    Time ________________________________>
    //         ^   ^      ^   ^
    //        *0   *1     *2  *3
    //      *0: TaskDelay()呼び出し
    //      *1: Delay開始
    //      *2: Delay終了
    //      *3: TaskDelay呼び出し
    //
    //   各ポイント間の時間は次の通り.
    //     *0 ~ *1: ほかタスクの実行時間
    //     *1 ~ *2: プログラマが設定した時間(IntervalTime).
    //     *2 ~ *3: ほかタスクの実行時間. 次のループ先にあるDelayにたどり着く時間
    //
    //   以上からわかるようにDelay()と次回ループのDelay()の時間間隔はIntervalTimeであるべきだが,
    //   この場合, それ以上の時間(*0 ~ *3)がかかる.
    //
    // @param ticksToDelay:
    //  The amount of time, in tick periods, that the calling task should block.
    //
    // Example usage:

    void TaskFunction(void *parameters)
    {
        // Block for 500ms
        const PortTIckType delayTime = 500 / PORT_TICK_RATE_MS;

        for (;;)
        {
            // Simply toggle the LED every 500ms, blocking between each toggle.
            ToggleLED();
            TaskDelay(delayTime);
        }
    }
    */
    void TaskDelay(PortTickType ticksToDelay);

    /*
    //
    // INCLUDE_TASK_DELAY_UNTIL must be defined as 1 for this function to be available.
    // See the configuration section for more information.
    //
    // Delay a task until a specified time. This function can be used by cyclical
    // tasks to ensure a constant execution frequency.
    //
    // This function differs from vTaskDelay() in one important aspect : vTaskDelay() will
    // cause a task to block for the specified number of ticks from the time vTaskDelay() is
    // called.It is therefore difficult to use vTaskDelay() by itself to generate a fixed
    // execution frequency as the time between a task starting to execute and that task
    // calling vTaskDelay() may not be fixed[the task may take a different path though the
    // code between calls, or may get interrupted or preempted a different number of times
    // each time it executes].
    //
    // Whereas TaskDelay() specifies a wake time relative to the time at which the function
    // is called, TaskDelayUntil() specifies the absolute(exact) time at which it wishes to
    // unblock.
    //
    // The constant PORT_TICK_RATE_MS can be used to calculate real time from the tick
    // rate - with the resolution of one tick period.
    //
    // @param previousWakeTime:
    //  Pointer to a variable that holds the time at which the
    //  task was last unblocked.The variable must be initialised with the current time
    //  prior to its first use(see the example below).Following this the variable is
    //  automatically updated within TaskDelayUntil().
    //
    // @param timeIncrement:
    //  The cycle time period.The task will be unblocked at
    //  time *previousWakeTime + timeIncrement.Calling TaskDelayUntil with the
    //  same timeIncrement parameter value will cause the task to execute with
    //  a fixed interface period.
    //
    // Example usage :

    void TaskFunction(void *parameters)
    {
        PortTickType lastWakeTime;
        const PortTIckType frequency = 10;

        // Initialise the lastWakeTime variable with the current time.
        lastwakeTime = TaskGetTickCount();
        for (;;)
        {
            // Wait for the next cycle.
            TaskDelayUntil(&lastWakeTime, frequency);

            // Perform action here.
        }
    }
    */
    void TaskDelayUntil(PortTickType * const previouswakeTime, PortTickType timeIncrement);

    /*
    //
    // INCLUDE_TASK_GET_STATE must be defined as 1 for this function to be available.
    // See the configuration section for more information.
    //
    // Obtain the state of any task.  States are encoded by the TaskState
    // enumerated type.
    //
    // @param task:
    //  Handle of the task to be queried.
    //
    // @return
    //  The state of xTask at the time the function was called.  Note the
    //   state of the task might change between the function being called, and the
    //   functions return value being tested by the calling task.
    */
    TaskState TaskGetState(TaskHandle task);

    /*
    // INCLUDE_TASK_SUSPEND must be defined as 1 for this function to be available.
    // See the configuration section for more information.
    //
    // Suspend any task.  When suspended a task will never get any microcontroller
    // processing time, no matter what its priority.
    //
    // Calls to TaskSuspend are not accumulative -
    // i.e. calling TaskSuspend () twice on the same task still only requires one
    // call to TaskResume () to ready the suspended task.
    //
    // @param taskToSuspend:
    //  Handle to the task being suspended.  Passing a NULL
    //  handle will cause the calling task to be suspended.
    //
    // Example usage:

    void AFunction(void)
    {
        TaskHandle handle;

        // Create a task, storing the handle.
        TaskCreate(TaskCodeFunction, "NAME", STACK_SIZE, NULL, IDLE_TASK_PRIORITY, &handle);

        // ...

        // Use the handle to suspend the created task.
        TaskSuspend(handle);

        // ...

        // Suspend ourselves.
        TaskSuspend(NULL);

        // We cannot get here unless another task calls TaskResume
        // with our handle as the parameter.
    }
    */
    void TaskSuspend(TaskHandle taskToSuspend);

    /*
    //
    // INCLUDE_TASK_SUSPEND must be defined as 1 for this function to be available.
    // See the configuration section for more information.
    //
    // Resumes a suspended task.
    //
    // A task that has been suspended by one of more calls to TaskSuspend ()
    // will be made available for running again by a single call to
    // TaskResume ().
    //
    // @param:
    //  taskToResume Handle to the task being readied.
    //
    // Example usage:


    void AFunction(void)
    {
        TaskHandle handle;

        // Create a task, storing the handle.
        TaskCreate(TaskCodeFunction, "NAME", STACK_SIZE, NULL, IDLE_TASK_PRIORITY, &handle);

        // ...

        // Use the handle to suspend the created task.
        TaskSuspend(handle);

        // ...

        // Task created task will not run during this period, unless
        // another task calls TaskResume(handle).

        // ...

        // Resume the suspended task ourselves.
        TaskResume(handle);

        // The created task will once again get microcontroller processing
        // time in accordance with it priority within the system.
    }
    */
    void TaskResume(TaskHandle taskToResume);

    //
    // INCLUDE_TASK_RESUME_FROM_ISR must be defined as 1 for this function to be
    // available.See the configuration section for more information.
    //
    // An implementation of TaskResume() that can be called from within an ISR.
    //
    // A task that has been suspended by one of more calls to vTaskSuspend()
    // will be made available for running again by a single call to
    // TaskResumeFromISR().
    //
    // @param taskToResume:
    //  Handle to the task being readied.
    //
    PortBaseType TaskResumeFromISR(TaskHandle taskToResume);

    // END TASK CONTROL API ----------------------------------------

    // ---------------------------------------------------------------------
    // SCHEDULER CONTROL
    // ---------------------------------------------------------------------


    /*
    // Starts the real time kernel tick processing.  After calling the kernel
    // has control over which tasks are executed and when.  This function
    // does not return until an executing task calls TaskEndScheduler ().
    //
    // At least one task should be created via a call to TaskCreate ()
    // before calling TaskStartScheduler ().  The idle task is created
    // automatically when the first application task is created.
    //
    // Example usage:

    void Function(void)
    {
        // Create at least one task before starting the kernel.
        TaskCreate(TaskCodeFunction, "NAME", STACK_SIZE, NULL, IDLE_TASK_PRIORITY, NULL);

        // Start the real time kernel with preemption.
        TaskStartScheduler();

        // Will not get here unless a task calls TaskEndScheduler().
    }
    */
    void TaskStartScheduler(void);


    /*
    // Stops the real time kernel tick.  All created tasks will be automatically
    // deleted and multitasking (either preemptive or cooperative) will
    // stop.  Execution then resumes from the point where TaskStartScheduler ()
    // was called, as if TaskStartScheduler () had just returned.
    //
    // TaskEndScheduler () requires an exit function to be defined within the
    // portable layer (see PortEndScheduler () in port. c for the PC port).  This
    // performs hardware specific operations such as stopping the kernel tick.
    //
    // TaskEndScheduler () will cause all of the resources allocated by the
    // kernel to be freed - but will not free resources allocated by application
    // tasks.
    //
    // Example usage:

    void TaskCodeFunction(void *parameters)
    {
        for (;;)
        {
            // Task code goes here.

            // At some point we want to end the real time kernel processing
            // so call...
            TaskEndScheduler();
        }
    }

    void Function(void)
    {
        // Create at least one task before startig the kernel.
        TaskCreate(TaskCodeFunction, "NAME", STACK_SIZE, NULL, IDLE_TASK_PRIORITY, NULL);

        // Start the real time kernel with preemption.
        TaskStartScheduler();

        // Will only get here when the TaskCodeFunction() task has called
        // TaskEndScheduler(). When we get here we are back to single task
        // execution.
    }
    */
    void TaskEndScheduler(void);

    /*
    // Suspends all real time kernel activity while keeping interrupts (including the
    // kernel tick) enabled.
    //
    // After calling TaskSuspendAll () the calling task will continue to execute
    // without risk of being swapped out until a call to TaskResumeAll () has been
    // made.
    //
    // API functions that have the potential to cause a context switch (for example,
    // TaskDelayUntil(), QueueSend(), etc.) must not be called while the scheduler
    // is suspended.
    //
    // Example usage:


    void Task1(void *parameters)
    {
        for (;;)
        {
            // Task code goes here.

            // ...

            // At some point the task wants to perform a long operation during
            // which it does not want to get swapped out.  It cannot use
            // TaskEnterCritical ()/TaskExitCritical() () as the length of the
            // operation may cause interrupts to be missed - including the
            // ticks.

            // Prevent the real time kernel swapping out the task.
            TaskSuspendAll();

            // Perform the operation here.  There is no need to use critical
            // sections as we have all the microcontroller processing time.
            // During this time interrupts will still operate and the kernel
            // tick count will be maintained.

            // ...

            // The operation is complete. Restart the kerneel.
            TaskResumeAll();
        }
    }
    */
    void TaskSuspendAll(void);

    /*
    // Resumes real time kernel activity following a call to TaskSuspendAll().
    // After a call to TaskSuspendAll() the kernel will take control of which
    // task is executing at any time.
    //
    // @return:
    //  If resuming the scheduler caused a context switch then pdTRUE is
    //  returned, otherwise pdFALSE is returned.
    //
    // Example usage :

    void Task1(void *parameters)
    {
        for (;;)
        {
            // Task code goes here.

            // ...

            // At some point the task wants to perform a long operation during
            // which it does not want to get swapped out.  It cannot use
            // TaskEnterCritical ()/TaskExitCritical() () as the length of the
            // operation may cause interrupts to be missed - including the
            // ticks.

            // Prevent the real time kernel swapping out the task.
            TaskSuspendAll();

            // Perform the operation here.  There is no need to use critical
            // sections as we have all the microcontroller processing time.
            // During this time interrupts will still operate and the real
            // time kernel tick count will be maintained.

            // ...

            // The operation is complete.  Restart the kernel.  We want to force
            // a context switch - but there is no point if resuming the scheduler
            // caused a context switch already.
            if (!TaskResumeAll())
            {
                TaskYield();
            }
        }
    }
    */
    signed PortBaseType TaskResumeAll(void);

    /*
    //
    // Utility task that simply returns PD_TRUE if the task referenced by task is
    // currently in the Suspended state, or PD_FALSE if the task referenced by task
    // is in any other state.
    //
    */
    signed PortBaseType TaskIsTaskSuspended(TaskHandle task);

    // END SCHEDULER CONTROL ----------------------------

    //-------------------------------------------------------------------
    // TASK UTILITIES
    //-------------------------------------------------------------------

    //
    // @return:
    //  The count of ticks since TaskStartScheduler was called.
    //
    PortTickType TaskGetTickCount(void);

    //
    // @return:
    //  The count of ticks since TaskStartScheduler was called.
    //
    // This is a version of TaskGetTickCount() that is safe to be called from an
    // ISR - provided that portTickType is the natural word size of the
    // microcontroller being used or interrupt nesting is either not supported or
    // not being used.
    //
    PortTickType TaskGetTickCountFromISR(void);

    /*
    // @return:
    //  The number of tasks that the real time kernel is currently managing.
    //  This includes all ready, blocked and suspended tasks.  A task that
    //  has been deleted but not yet freed by the idle task will also be
    //  included in the count.
    */
    unsigned PortBaseType TaskGetNumberOfTasks(void);

    //-------------------------------------------------------------------
    // SCHEDULER INTERNALS AVAILABLE FOR PORTING PURPOSES
    //-------------------------------------------------------------------

    //
    // THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS ONLY
    // INTENDED FOR USE WHEN IMPLEMENTING A PORT OF THE SCHEDULER AND IS
    // AN INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
    // 
    // Called from the real time kernel tick (either preemptive or cooperative),
    // this increments the tick count and checks if any tasks that are blocked
    // for a finite period required removing from a blocked list and placing on
    // a ready list.
    //
    void TaskIncrementTick(void);


    /*
    // 警告:
    //  この関数はKernel内で用いられる関数です. アプリケーションでは, 実行しないでください.
    //
    // 現在のタスクをreadyListから除外し, eventListとdelayListに登録します.
    // eventListでは, 優先度が高い順番に登録されます.
    //
    // THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS AN
    // INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
    //
    // THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED.
    //
    // Removes the calling task from the ready list and places it both
    // on the list of tasks waiting for a particular event, and the
    // list of delayed tasks.  The task will be removed from both lists
    // and replaced on the ready list should either the event occur (and
    // there be no higher priority tasks waiting on the same event) or
    // the delay period expires.
    //
    // @param eventList:
    //  The list containing tasks that are blocked waiting
    //  for the event to occur.
    //
    // @param ticksToWait:
    //  The maximum amount of time that the task should wait
    //  for the event to occur.  This is specified in kernel ticks,the constant
    //  PORT_TICK_RATE_MS can be used to convert kernel ticks into a real time
    //  period.
    //
    */
    void TaskPlaceOnEventList(const List * const eventList, PortTickType ticksToWait);

    /*
    //
    // 警告:
    //  この関数はKernel内で用いられる関数です. アプリケーションでは, 実行しないでください.
    //
    // eventList先頭にあるタスクをeventListとdelayedListから除外し, readyListに登録します.
    // eventListの先頭にあるタスクは優先度が高いタスクとなります.
    //
    // THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS AN
    // INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
    //
    // THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED.
    //
    // Removes a task from both the specified event list and the list of blocked
    // tasks, and places it on a ready queue.
    //
    // TaskRemoveFromEventList () will be called if either an event occurs to
    // unblock a task, or the block timeout period expires.
    //
    // @return pdTRUE:
    //  if the task being removed has a higher priority than the task
    //  making the call, otherwise pdFALSE.
    //
    */
    signed PortBaseType TaskRemoveFromEventList(const List * const eventList);

    // THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.
    // IT IS ONLY INTENDED FOR USE WITH IMPLEMENTING A PORT OF THE SCHEDULER
    // AND IS AN INTERFACE WHICH IS FOR THE FUNCTION USE OF THE SCHEDULER.
    //
    // Sets the pointer to the current TCB to the TCB of the highest priority rask
    // that is ready to run.
    void TaskSwitchContext(void);

    //
    // Return the handle of the calling task.
    //
    TaskHandle TaskGetCurrentTaskHandle(void);

    //
    // Capture the current time status for future refernce.
    //
    void TaskSetTimeOutState(TimeOutType * const timeOut);

    //
    // Compare the time status now with that previously captured so see if the
    // timeout has expired.
    //
    PortBaseType TaskCheckForTimeOut(TimeOutType * const timeOut, PortTickType * const ticksToWait);

    //
    // Shortcut used by the queue implementation to prevent unnecessary call to
    // TaskYield();
    //
    void TaskMissedYield(void);

    //
    // Returns the scheduler state is as
    // TASK_SCHEDULER_NOT_STARTED, TASK_SCHEDULER_RUNNING, TASK_SCHEDULER_SUSPENDED.
    //
    PortBaseType TaskGetSchedulerState(void);

    // 
    // 優先度継承
    // 優先度継承とは, 優先順位の逆転を防ぐ手法の一種である.
    // 基本的な考え方は, Jというジョブがより高優先度のジョブ群をブロックしているとき,
    // Jが本来の優先度ではなくそれらジョブ(高優先度のジョブ)と同等の優先度で動作することである.
    // Jがクリティカルセクションの実行(ここでいうクリティカルセクションとは全割り込みを停止するものではなく,
    // セマフォによる排他制御を行う部分である)を終えたとき, Jは本来の優先度に戻る
    //
    // Raises the priority of the mutex holder to that of the calling task
    // should the mutex holder have a priority less than the calling task.
    //
    void TaskPriorityInherit(TaskHandle * const mutexHolder);

    //
    // Set the priority of a task back to its proper priority in the case that it
    // inherited a higher priority while it was holding a semaphore.
    //
    void TaskPriorityDisinherit(TaskHandle * const mutexHolder);

    /*
    // Generic versions of the task creation function which is in turn called by the
    // TaskCreate() macro.
    */
    signed PortBaseType TaskGenericCreate(TaskCode taskCode, const signed char *const name,
        unsigned short stackDepth, void *parameters, unsigned PortBaseType priority,
        TaskHandle *createdTask, PortStackType *stackBuffer);


#ifdef __cplusplus
}
#endif
#endif