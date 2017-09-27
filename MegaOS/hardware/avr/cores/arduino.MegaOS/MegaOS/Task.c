
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


//Standard includes
#include <stdlib.h>
#include <string.h>

// MegaOS includes
#include "MegaOS.h"
#include "Task.h"
//#include "Timers.h"
#include "StackMacros.h"

// Defines the size, in words, of the stack allocated to the idle task.
#define IDLE_TASK_STACK_SIZE CONFIG_MINIMAL_STACK_SIZE

// 
// Task control block
// A task control block (TCB) is allocated for each task,
// and stores task state information, includeing a pointer to the task's context
// (the task's run time environment, includeing register values)
//
typedef struct 
{
    // Points to the location of the last item placed on the tasks stack.
    // THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT.
    // 
    // --- memo ---
    // スタックポインタは常にスタックの先頭を指している.
    //
    volatile PortStackType *topOfStack;

    // The list that the state list item of a task is reference from denotes 
    // the state of that task (Ready, Blocked, Suspended).
    ListItem genericListItem;

    // Used to reference a task from an event list.
    ListItem eventListItem;

    // The priority of the task. 0 is the lowest priority.
    unsigned PortBaseType priority;

    // Points to the start of the stack.
    PortStackType *stack;

    // Descriptive name given to the task when created. Facilitates debugging only.
    signed char taskName[CONFIG_MAX_TASK_NAME_LEN];

    #if(PORT_STACK_GROWTH > 0)
        // Points to the end of the stack or architectures where the stack grows up
        // from low memory. 
        PortStackType *endOfStack;
    #endif

    #if(CONFIG_USE_MUTEXES == 1)
        // The priority last assigned to the task - used by the priority inheritance machanism.
        unsigned PortBaseType basePriority;
    #endif
}TaskControlBlock;

TaskControlBlock* volatile currentTCB = NULL;

// ---Lists for ready and blocked tasks.---------------

// Prioritised ready tasks.
static List readyTasksLists[CONFIG_MAX_PRIORITIES];

// Delayed tasks.
static List delayedTaskList1;

// Delayed tasks (two lists are used - one for delays that have 
// overflowed the current tick count.
static List delayedTaskList2;

// Points to the delayed task list currently being used.
static List * volatile delayedTaskList;

// Points to the delayed task list currently being used to hold tasks 
// that have overflowed the current tick count.
static List * volatile overflowDelayedTaskList;

// Tasks that have been readied while the scheduler was suspended.
// They will be moved to the ready queue when the scheduler is resumed.
static List pendingReadyList;

#if (INCLUDE_TASK_DELETE == 1)
    // Tasks that have been deleted - but the theirir memory not yet freed.
    static List tasksWaitingTermination;
    static volatile unsigned PortBaseType tasksDeleted = (unsigned PortBaseType)0U;
#endif

#if (INCLUDE_TASK_SUSPEND == 1)
    // Tasks that are currently suspended.
    static List suspendedTaskList;
#endif

// ----------------------------------------------------
// --- File private variables. ----------------------------------------
static volatile unsigned PortBaseType currentNumberOfTasks = (unsigned PortBaseType) 0U;
static volatile PortTickType tickCount = (PortTickType)0U;
static unsigned PortBaseType topUsedPriority = IDLE_TASK_PRIORITY;
static volatile unsigned PortBaseType topReadyPriority = IDLE_TASK_PRIORITY;
static volatile signed PortBaseType schedulerRunning = PD_FALSE;
static volatile unsigned PortBaseType schedulerSuspended = (unsigned PortBaseType)PD_FALSE;
static volatile unsigned PortBaseType missedTicks = (unsigned PortBaseType)0U;
static volatile PortBaseType missedYield = (PortBaseType)PD_FALSE;
static volatile PortBaseType numOfOverflows = (PortBaseType)0;
static unsigned PortBaseType taskNumber = (unsigned PortBaseType)0U;

// 
static volatile PortTickType nextTaskUnblockTime = (PortTickType)PORT_MAX_DELAY;
// -----------------------------------------------------

// --- Debuging ------------------------------------------------------
// The value used to fill the stack of a task when the task is created.
// This is used purely for checking the high water mark for tasks.
//
// Value: 1010 0101
#define TASK_STACK_FILL_BYTE (0xa5U)
// ------------------------------------------------------

// 
// -----------------------------------------------------------------
// TASK SELECTION
// -----------------------------------------------------------------

// TopReadyPriority holds the priority of the highest priority ready state task.
#define TaskRecordReadyPriority(priority)                           \
{                                                                   \
    if((priority) > topReadyPriority)                               \
    {                                                               \
        topReadyPriority = (priority);                              \
    }                                                               \
}

#define TaskSelectHighestPriorityTask()                                         \
{                                                                               \
    /* Find the highest priority queue that contains ready tasks.*/             \
    /* indexが負であることの判定をしない理由 */                                 \
    while(ListListIsEmpty(&(readyTasksLists[topReadyPriority])))                \
    {                                                                           \
        --topReadyPriority;                                                     \
    }                                                                           \
    /* ListGetOwnerOfNextEntry indexes through the list,    */                  \
    /* so the tasks of the same priority get an equal share */                  \
    /* of the processor time                                */                  \
    ListGetOwnerOfNextEntry(currentTCB, &(readyTasksLists[topReadyPriority]));  \
}

// --- End TASK SELECTION--------------------------------------------

//
// Place the task reperesented by TCB into the appropriate ready queue for
// the task. It is inserted at the end of the list. Obe quirk of this is
// that if the task being inserted is at the same priority as the currently
// executing task, then it willonly be rescheduled after the currently
// executing task has been rescheduled.
//
#define AddTaskToReadyQueue(tcb)                                                            \
    TaskRecordReadyPriority((tcb)->priority);                                               \
    ListInsertEnd((List *)&(readyTasksLists[(tcb)->priority]), &((tcb)->genericListItem))   

//
// Macro that looks at the list of tasks that are currently delayed to see if
// any require waking.
//
// Tasks are stored in the queue in the order of their wake time - meaning
// once one tasks has been found whose timer has no expired we need not look
// any future down the list.
//
#define CheckDelayedTasks()                                                         \
{                                                                                   \
    PortTickType itemValue;                                                         \
                                                                                    \
    /* Is the tick count greater than or equal to the wake time of the */           \
    /* first task refernced from the delayed tasks list? */                         \
    if(tickCount >= nextTaskUnblockTime)                                            \
    {                                                                               \
        for(;;)                                                                     \
        {                                                                           \
            if(ListListIsEmpty(delayedTaskList) != PD_FALSE)                        \
            {                                                                       \
                /* The delayed list is empty. Set nextTaskUnblockTime */            \
                /* to the maximum possible value so it is extremely */              \
                /* unlikely that the if(tickCount >= nextTaskUnblockTime) */        \
                /* test will pass next time through. */                             \
                nextTaskUnblockTime = PORT_MAX_DELAY;                               \
                break;                                                              \
            }                                                                       \
            else                                                                    \
            {                                                                       \
                /* The delayed list is not empty, get the value of the */           \
                /* at the head of the delayed list. This is the time at */          \
                /* which the task at the head of the delayed list should */         \
                /* be removed from the Block state. */                              \
                tcb = (TaskControlBlock *)ListGetOwnerOfHeadEntry(delayedTaskList); \
                itemValue = ListGetListItemValue(&(tcb->genericListItem));          \
                                                                                    \
                if(tickCount < itemValue)                                           \
                {                                                                   \
                    /* It is not time to unblock this item yet, but the item */     \
                    /* value is the time at which the task at the head of the */    \
                    /* blocked list should be removed from the Blocked state - */   \
                    /* so record the item value in nextTaskUnblockTime. */          \
                    nextTaskUnblockTime = itemValue;                                \
                    break;                                                          \
                }                                                                   \
                                                                                    \
                /* It is time to remove the item from the Blocked state. */         \
                ListRemove(&(tcb->genericListItem));                                \
                                                                                    \
                /* Is the task waiting on an event also? */                         \
                if(tcb->eventListItem.container != NULL)                            \
                {                                                                   \
                    ListRemove(&(tcb->eventListItem));                              \
                }                                                                   \
                AddTaskToReadyQueue(tcb);                                           \
            }                                                                       \
        }                                                                           \
    }                                                                               \
}

//
// TaskHandle―アプリケーション側から送られる―からTCBを求める.
// NULLが渡された場合は現在実行中のTCBが返される.
//
// Several function task an TaskHandle parameter that can optionally be NULL,
// where NULL is used to indicate that the handle of the currently executing
// task should be used in place of the parameter. This macro simply checks to
// see if the parameter is NULL and returns a pointer to the appropriate TCB.
//
#define GetTCBFromHandle(handle) (((handle) == NULL) ? (TaskControlBlock *)currentTCB : (TaskControlBlock *)(handle))

// Callback function prototypes
extern void ApplicationStackOverflowHook(TaskHandle task, signed char *taskName);
extern void ApplicationTickHook(void);

// --- File private functions. ---------------------------------------------------

//
// Utility to ready a TCB for a given task. Mainly just copies the parameters
// into the TCB structure.
//
static void InitialiseTCBVariables(TaskControlBlock *tcb, const signed char * const name, unsigned PortBaseType, unsigned short stackDepth);

//
// Utility to ready all the lists used by the scheduler. This is called
// automatically upon the creation of the first task.
//
static void InitialiseTaskLists(void);

// 
// The idle task, which as all tasks is implemented as a never ending loop.
// The idle task is automatically created and added to the ready lists upon
// creation of the first user task.
//
// The PortTaskFunctionProto() macro is used to allow port/compiler specific
// language extensions. The equivalent prototype for this function is:
//
// void IdleTask(void *parameters)
// 
static PortTaskFunctionProto(IdleTask, parameters);

//
// Utility to free all memory allocated by the scheduler to hokd a TCB,
// includeing the stack pointed to by the TCB.
// 
// This does not free memory allocated by the task itself
// (i.e. memory allocated by calls to PortMalloc from whithin the tasks application code).
#if (INCLUDE_TASK_DELETE == 1)
    static void DeleteTCB(TaskControlBlock *tcb);
#endif

// 
// Used only by the idle task. This checks to see if anything has been placed
// in the list of tasks waiting to be deleted. If so the task is cleaned up
// and its TCB deleted.
//
static void CheckTasksWaitingTermination(void);

// 
// The currently executing task is entering the Blocked state. Add the task to
// either the current or the overflow delayed task list.
//
static void AddCurrentTaskToDelayedList(PortTickType timeToWake);


// Allocates memory from the heap for a TCB and associated stack.
// Checks the allocation was successful.
static TaskControlBlock* AllocateTCBAndStack(unsigned short stackDepth, PortStackType *stackBuffer);



signed PortBaseType TaskGenericCreate(TaskCode taskCode, const signed char *const name,
    unsigned short stackDepth, void *parameters, unsigned PortBaseType priority,
    TaskHandle *createdTask, PortStackType *stackBuffer)
{
    signed PortBaseType ret;
    TaskControlBlock *newTCB;

    // TCB用のメモリを確保
    // Allocate the memory required by the TCB and stack for the new task,
    // checking that the allocation was successful.
    newTCB = AllocateTCBAndStack(stackDepth, stackBuffer);

    if (newTCB != NULL)
    {
        PortStackType *topOfStack;

        // Calculate the top of stack address. This depends on whether the 
        // stack grows from high memory to low (as per the 80x86) or visa versa.
        // PORT_STACK_GROWTH is used to make the result positive or negative as
        // required by the port.
        #if(PORT_STACK_GROWTH < 0)
        {
            topOfStack = newTCB->stack + (stackDepth - (unsigned short)1);
            topOfStack = (PortStackType *)(((PortPointerSizeType)topOfStack)
                            & ((PortPointerSizeType)~PORT_BYTE_ALIGNMENT_MASK));
        }
        #else
        {
            topOfStack = newTCB->stack;

            // If we want to use stack checking on architectures the use
            // a positive stack growth direction then we also need to store the
            // other extreme of the stack space.
            newTCB->endOfStack = newTCB->stack + (stackDepyh - 1);
        }
        #endif

        // Setup the newly allocated TCB with the initial state of the task.
        InitialiseTCBVariables(newTCB, name, priority, stackDepth);

        // Initialize the TCB stack to look as if the task was already runnning,
        // but had been interrupted by the scheduler. The return address is set
        // to the start of the task function. Once the stack has been initialised
        // the top of stack variable is updated.
        newTCB->topOfStack = PortInitialiseStack(topOfStack, taskCode, parameters);

        // 引数にハンドル変数があるとき.
        if ((void *)createdTask != NULL)
        {
            // Pass the TCB out - in an anonymous way. The calling function/
            // task can use this as a handle to delete the task later if
            // reqired.
            // ハンドル変数のメモリアドレスにtcbのメモリアドレスを代入する.
            *createdTask = (TaskHandle)newTCB;
        }

        // We are going to manipulate the task queues to add this task to a ready
        // list, so must make sure no interrupts occur.
        TaskEnterCritical();
        {
            currentNumberOfTasks++;
            if (currentTCB == NULL)
            {
                // There are no other tasks, or all the other tasks are 
                // in the suspended state - make this the current task.
                currentTCB = newTCB;

                if (currentNumberOfTasks == (unsigned PortBaseType)1)
                {
                    // This is the first task to be created so do the preliminary
                    // initialisation required. We will not recover if this call
                    // fails, but we report the failure.
                    InitialiseTaskLists();
                }
            }
            else
            {
                // If the scheduler is not already running, make this task the
                // current task if it is the highest priority task to be created
                // so far.
                if (schedulerRunning == PD_FALSE)
                {
                    if (currentTCB->priority <= priority)
                    {
                        currentTCB = newTCB;
                    }
                }
            }

            // Remember the top priority to make context switching faster.
            // Use the priority in newTCB as this has been capped to a valid value.
            if (newTCB->priority > topUsedPriority)
            {
                topUsedPriority = newTCB->priority;
            }

            taskNumber++;

            TraceTaskCreate(newTCB);

            AddTaskToReadyQueue(newTCB);
            ret = PD_PASS;

        }
        TaskExitCritical();
    }
    else
    {
        ret = ERR_COULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        TraceTaskCreateFailed();
    }

    if (ret == PD_PASS)
    {
        if (schedulerRunning != PD_FALSE)
        {
            // If the created task is of a higher priority than the current task
            // then it should run now.
            if (currentTCB->priority < priority)
            {
                PortYieldWithinAPI();
            }
        }
    }

    return ret;
}

#if (INCLUDE_TASK_DELETE == 1)
void TaskDelete(TaskHandle taskToDelete)
{
    TaskControlBlock *tcb;

    TaskEnterCritical();
    {
        // Ensure a yield is performed if the currentt task is being deleted.
        if (taskToDelete == currentTCB)
        {
            taskToDelete == NULL;
        }

        // If null is passed in herer then we are deleting ourselves.
        tcb = GetTCBFromHandle(taskToDelete);

        // Remove task from the ready list and place in the termination list.
        // This will stop the task from be scheduled. The idle task will check
        // the termination list and free up any memory allocated by the scheduler
        // for the TCB and stack.
        if (ListRemove((ListItem *)(&(tcb->genericListItem))) == 0)
        {

        }

        // Is the task waitng on an event also?
        if (tcb->eventListItem.container != NULL)
        {
            ListRemove(&(tcb->eventListItem));
        }

        ListInsertEnd((List *)&tasksWaitingTermination, &(tcb->genericListItem));

        // Increment the tasksDeleted variable so the idle task knows
        // there is a task that has been deleted and that it should therefore
        // check the taskWaitingTermination list.
        ++tasksDeleted;

        // Increment the taskNumberVariable also so kernel aware debuggers
        // can detect that the task lists need re-generating.
        taskNumber++;

        TraceTaskDelete(tcb);
    }
    TaskExitCritical();
    
    // Force a reschedule if we have just deleted the current task.
    if (schedulerRunning != PD_FALSE)
    {
        if ((void *)taskToDelete == NULL)
        {
            PortYieldWithinAPI();
        }
    }
}
#endif

#if(INCLUDE_TASK_DELAY_UNTIL == 1)
void TaskDelayUntil(PortTickType * const previousWakeTime, PortTickType timeIncrement)
{
    PortTickType timeToWake;
    PortBaseType alreadyYielded, shouldDelay = PD_FALSE;

    TaskSuspendAll(); 
    {
        // Generate the tick time at which the task wants to wake.
        timeToWake = *previousWakeTime + timeIncrement;

        if (tickCount < *previousWakeTime)
        {
            // The tick count has overflowed since this function was
            // lasted called. In this case the only time we should ever
            // actually delay is if the wake time has also overflowed,
            // and the wake time is greater than the tick time. When this
            // is the case it is as if neither time had overflowed.
            if ((timeToWake < *previousWakeTime) && (timeToWake > tickCount))
            {
                shouldDelay = PD_TRUE;
            }
        }
        else
        {
            // The tick time has not overflowed. In this case we will
            // delay if either the wake time has overflowed, and/or the
            // tick time is less than the wake time.
            if ((timeToWake < *previousWakeTime) || (timeToWake > tickCount))
            {
                shouldDelay = PD_TRUE;
            }
        }

        // Update the wake time ready for the next call.
        *previousWakeTime = timeToWake;

        if (shouldDelay != PD_FALSE)
        {
            TraceTaskDelayUntil();

            // We must remove ourselves from the ready list before adding
            // ourselves to the blocked list as the same list item is used for
            // both lists.
            if (ListRemove((ListItem *)&(currentTCB->genericListItem)) == 0)
            {

            }
            AddCurrentTaskToDelayedList(timeToWake);
        }
    }
    alreadyYielded = TaskResumeAll();

    // Force a reschedule if TaskResumeAll has not already done so, we may
    // have put ourselves to sleep.
    if (alreadyYielded == PD_FALSE)
    {
        PortYieldWithinAPI();
    }
}
#endif

#if(INCLUDE_TASK_DELAY == 1)
void TaskDelay(PortTickType ticksToDelay)
{
    PortTickType timeToWake;
    signed PortBaseType alreadyYield = PD_FALSE;

    // A delay time of zero just forces a reschedule.
    if (ticksToDelay > (PortTickType)0U)
    {
        TaskSuspendAll();
        {
            TraceTaskDelay();

            // A task that is removed from the event list while the
            // scheduler is suspended will not get placed in the ready
            // list or removed from the blocked list until the scheduler 
            // is resumed.
            //
            // That task cannot be in an event list as it is the currently
            // executing task.
            
            // Calculate the time to wake - this may overflow but this is
            // not a problem.
            timeToWake = tickCount + ticksToDelay;

            // We must remove ourselves from the ready list before adding
            // ourselves to the blocked list as the same list item is used for
            // both lists.
            if (ListRemove((ListItem *)&(currentTCB->genericListItem)) == 0)
            {

            }
            AddCurrentTaskToDelayedList(timeToWake);
        }
        alreadyYield = TaskResumeAll();
    }

    // Force a reschedule if TaskResumeAll has not already done so, we may
    // have put ourselves to sleep.
    if (alreadyYield == PD_FALSE)
    {
        PortYieldWithinAPI();
    }
}
#endif

#if (INCLUDE_TASK_GET_STATE == 1)
TaskState TaskGetState(TaskHandle task)
{
    TaskState ret;
    List *stateList;
    TaskControlBlock *tcb;

    tcb = (TaskControlBlock *)task;

    if (tcb == currentTCB)
    {
        // The task calling this function is querying its own state.
        ret = Running;
    }
    else
    {
        TaskEnterCritical();
        {
            stateList = (List *)ListListItemContainer(&(tcb->genericListItem));
        }
        TaskExitCritical();

        if ((stateList == delayedTaskList) || (stateList == overflowDelayedTaskList))
        {
            // The task being queried is referenced from one of the blocked lists.
            ret = Blocked;
        }

        #if (INCLUDE_TASK_SUSPEND == 1)
        else if (stateList == &suspendedTaskList)
        {
            // The task being queried is refrenced from the suspended list.
            ret = Suspended;
        }
        #endif

        #if (INCLUDE_TASK_DELETE == 1)
        else if (stateList == &tasksWaitingTermination)
        {
            // the task being queried is refrenced from the deleted tasks list.
            ret = Deleted;
        }
        #endif

        else
        {
            // If the task is not in any other state, it must be in the Ready
            // (including pending ready) state.
            ret = Ready;
        }
    }

    return ret;
}
#endif

#if (INCLUDE_TASK_SUSPEND == 1)
void TaskSuspend(TaskHandle taskToSuspend)
{
    TaskControlBlock *tcb;

    TaskEnterCritical();
    {
        // Ensure a yield is performed if the current task is being suspended.
        if (taskToSuspend == (TaskHandle)currentTCB)
        {
            taskToSuspend = NULL;
        }

        // If null is passed in here then we are suspending ourselves.
        tcb = GetTCBFromHandle(taskToSuspend);

        TraceTaskSuspend(tcb);

        // Remove task from the ready/delayed list and place in the suspended list.
        if (ListRemove((ListItem *)(&(tcb->genericListItem))) == 0)
        {

        }

        // Is the task waiting on an event also?
        if (tcb->eventListItem.container != NULL)
        {
            ListRemove(&(tcb->eventListItem));
        }

        ListInsertEnd((List *)&suspendedTaskList, &(tcb->genericListItem));
    }
    TaskExitCritical();

    if ((void *)taskToSuspend == NULL)
    {
        if (schedulerRunning != PD_FALSE)
        {
            // We have just suspended the current task.
            PortYieldWithinAPI();
        }
        else
        {
            //
            // サスペンド指定された現在実行中のタスクを停止し, ほかのタスクを実行する.
            // 
            // The scheduler is not running, but the task that was pointed
            // to by currentTCB has just been suspended and currentTCB
            // must be adjusted to point to a different task.
            if (ListCurrentListLength(&suspendedTaskList) == currentNumberOfTasks)
            {
                // すべてのタスクが休止状態のとき
                // No other tasks are ready, so  set currentTCB back to
                // NULL so when the next task is created currentTCB will
                // be set to point to it no matter what its relative priority 
                // is.
                currentTCB = NULL;
            }
            else
            {
                TaskSwitchContext();
            }
        }
    }
}
#endif

#if (INCLUDE_TASK_SUSPEND == 1)
signed PortBaseType TaskIsTaskSuspended(TaskHandle task)
{
    PortBaseType ret = PD_FALSE;
    const TaskControlBlock * const tcb = (TaskControlBlock *)task;

    // サスペンドタスクリストに入っているか?
    // It the task we are attempting toresume actually in the
    // suspended list?
    if (ListIsContainedWithin(&suspendedTaskList, &(tcb->genericListItem)) != PD_FALSE)
    {
        // Has the task already been resumed from within an ISR?
        if (ListIsContainedWithin(&pendingReadyList, &(tcb->eventListItem)) != PD_TRUE)
        {
            // Is it in the suspended list because it is in the
            // Suspended state? It is possible to be in the suspended
            // list because it is blocked on a task with no timeout 
            // specified.
            if (ListIsContainedWithin(NULL, &(tcb->eventListItem)) == PD_TRUE)
            {
                ret = PD_TRUE;
            }
        }
    }

    return ret;
}
#endif

#if (INCLUDE_TASK_SUSPEND == 1)
void TaskResume(TaskHandle taskToResume)
{
    TaskControlBlock *tcb;

    // Remove the task from whichever list is is currently in, and place
    // it in the ready list.
    tcb = (TaskControlBlock *)taskToResume;

    // The parameter cannot be NULL as it is impossible to resume  the 
    // currently executing task.
    if ((tcb != NULL) && (tcb != currentTCB))
    {
        TaskEnterCritical();
        {
            if (TaskIsTaskSuspended(tcb) == PD_TRUE)
            {
                TraceTaskResume(tcb);

                // As we are in a critical section we can access the ready
                // lsits even if the scheduler is suspended.
                ListRemove(&(tcb->genericListItem));
                AddTaskToReadyQueue(tcb);

                // We may have just resumed a higher priority task.
                if (tcb->priority >= currentTCB->priority)
                {
                    // This yield may not cause the task just resumed to run, but
                    // will leave the lsits in the correct state for the next yield.
                    PortYieldWithinAPI();
                }
            }
        }
        TaskExitCritical();
    }
}
#endif

#if ((INCLUDE_TASK_RESUME_FROM_ISR == 1) && (INCLUDE_TASK_SUSPEND == 1))
PortBaseType TaskResumeFromISR(TaskHandle taskToResume)
{
    PortBaseType yieldRequired = PD_FALSE;
    TaskControlBlock *tcb;

    tcb = (TaskControlBlock *)taskToResume;

    if (TaskIsTaskSuspended(tcb) == PD_TRUE)
    {
        TraceTaskResumeFromISR(tcb);

        if (schedulerSuspended == (unsigned PortBaseType)PD_FALSE)
        {
            yieldRequired = (tcb->priority >= currentTCB->priority);
            ListRemove(&(tcb->genericListItem));
            AddTaskToReadyQueue(tcb);
        }
        else
        {
            // We cannot access the delayed or ready lists, so will hold this
            // task pending until the scheduler is resumed, at which point a
            // yield will be performed if necessary.
            ListInsertEnd((List *)&(pendingReadyList), &(tcb->eventListItem));
        }
    }

    return yieldRequired;
}
#endif

void TaskStartScheduler(void)
{
    PortBaseType ret;

    // Add the idle task at the lowest priority
    ret = TaskCreate(IdleTask, (signed char) "IDLE", IDLE_TASK_STACK_SIZE, (void *)NULL, IDLE_TASK_PRIORITY, NULL);

    /*
    #if (CONFIG_USE_TIMERS == 1)
    {
        if (ret == PD_PASS)
        {
            ret = TimerCreateTimerTask();
        }
    }
    #endif
    */
    if (ret == PD_PASS)
    {
        // Interrupts are turned off here, to ensure a tick does not occur
        // before or during the call to PortStartScheduler(). The stacks of 
        // the created tasks contain a status word with interrupts switched on
        // so interrupts will automatically get re-enabled when the first task
        // starts to run.
        // 
        // STEPPING TROUGH HERE A DEBUGGER CAN CAUSE BIG PROBLEMS IF THE 
        // DEBUGGER ALLOWS INTERRUPTS TO BE PROCESSED.
        PortDisableInterrupts();

        schedulerRunning = PD_TRUE;
        tickCount = (PortTickType)0U;

        // Setting up the timer tick is hardware speciffic and thus in the 
        // portable interface.
        if (PortStartScheduler() != PD_FALSE)
        {
            // Should not reach here as if the scheduler is running the 
            // function will not return.
        }
        else
        {
            // Should onlt reach here if a task calls TaskEndScheduler().
        }
    }
    else
    {
        // This line will only be reached if the kernel could not started,
        // because there was not enough MegaOS heap to create the idle task
        // or the timer task.
    }
}

void TaskEndScheduler(void)
{
    // Stop the scheduler interrupts and call the portable scheduler end
    // routine so the original ISRs can be restore if necessary. The port
    // layer must ensure interrupts enable bit is left in the correct state.
    PortDisableInterrupts();
    schedulerRunning = PD_FALSE;
    PortEndScheduler();
}

void TaskSuspendAll(void)
{
    // この関数が呼ばれるごとにschedulerSuspendedをインクリメントすることで,
    // サスペンド要請回数を数えることができる.
    // A critical section is not required as the variable is of type PortBaseType
    ++schedulerSuspended;
}

signed PortBaseType TaskResumeAll(void)
{
    register TaskControlBlock *tcb;
    signed PortBaseType alreadyYielded = PD_FALSE;

    // If schedulerSuspended is zero the this function does not match a
    // previous call to TaskSuspendAll().

    // it is possible that an ISR caused a task to be removed from an event
    // list while rhe scheduler was suspended. If this was the case then the 
    // removed task will have been added to the pendingReadyList. Once the
    // scheduler has been resumed it is safe to move all the pending ready
    // tasks from this list into their appropriate ready list.
    TaskEnterCritical();
    {
        --schedulerSuspended;

        // サスペンド解除
        if (schedulerSuspended == (unsigned PortBaseType) PD_FALSE)
        {
            if (currentNumberOfTasks > (unsigned PortBaseType)0U)
            {
                PortBaseType yieldRequired = PD_FALSE;

                // Move any readied tasks from the pending list into the
                // appropriate ready list.
                while (ListListIsEmpty((List *)&pendingReadyList) == PD_FALSE)
                {
                    tcb = (TaskControlBlock *)ListGetOwnerOfHeadEntry(((List*)&pendingReadyList));
                    ListRemove(&(tcb->eventListItem));
                    ListRemove(&(tcb->genericListItem));
                    AddTaskToReadyQueue(tcb);

                    // If we have moved a task that has a priority higher than
                    // the current task then we should yield.
                    if (tcb->priority >= currentTCB->priority)
                    {
                        yieldRequired = PD_TRUE;
                    }
                }

                // If any ticks occurred while the scheduler was suspended then
                // they should be processed now. This ensures the tick count does not
                // slip, and that any delayed tasks are resumed at the correct time.
                if (missedTicks > (unsigned PortBaseType)0U)
                {
                    while (missedTicks > (unsigned PortBaseType)0U)
                    {
                        TaskIncrementTick();
                        --missedTicks;
                    }

                    // As we have processed some ticks it is appropriate to yield
                    // to ensure the highest priority task that is ready to run is
                    // the task actually running.
                    #if (CONFIG_USE_PREEMPTION == 1)
                    {
                        yieldRequired = PD_TRUE;
                    }
                    #endif
                }

                if ((yieldRequired == PD_TRUE) || (missedYield == PD_TRUE))
                {
                    alreadyYielded = PD_TRUE;
                    missedYield = PD_FALSE;
                    PortYieldWithinAPI();
                }
            }
        }
    }
    TaskExitCritical();

    return alreadyYielded;
}

PortTickType TaskGetTickCount(void)
{
    PortTickType ticks;

    // Critical section に入る理由:
    //  多くの場合PortTickTypeのバイト数はマイコンのレジスタ以上である.
    //  よって, tickCountを読み込むのに複数クロックかけなければならず,
    //  読み込み中に割り込みが入りtickがインクリメントされると, 読み込み中のデータが
    //  破損する危険性がある.
    //
    // Critical section required if running on a 16 bit processor.
    TaskEnterCritical();
    {
        ticks = tickCount;
    }
    TaskExitCritical();

    return ticks;
}

PortTickType TaskGetTickCountFromISR(void)
{
    PortTickType ret;

    ret = tickCount;

    return ret;
}

unsigned PortBaseType TaskGetNumberOfTasks(void)
{
    // A critical section is not required because the variables are of type
    // PortBaseType
    return currentNumberOfTasks;
}

void TaskIncrementTick(void)
{
    TaskControlBlock *tcb;

    // Called by the portable layer each time a tick interrupt occurs.
    // Increments the tick then checks to see if the new tick value will cause any task to be unblocked.
    if (schedulerSuspended == (unsigned PortBaseType)PD_FALSE)
    {
        ++tickCount;
        if (tickCount == (PortTickType)0U)
        {
            List *temp;

            // この時点でdelayedTaskListにitemがある場合がエラーが生じている.
            // Tick count has overflowed so we need to swap the delay lists.
            // If there are any items in delayedTaskList here then ther is an error!
            temp = delayedTaskList;
            delayedTaskList = overflowDelayedTaskList;
            overflowDelayedTaskList = temp;
            numOfOverflows++;

            // delayedListに変更があったため, これと連動しているnextTaskUnblockTime
            // も更新する必要がある.
            if (ListListIsEmpty(delayedTaskList))
            {
                // The new current delayed list is empty. Set 
                // nextTaskUnblockTime to the maxmum possible value so it is
                // extremely unlikely that the
                // if (tickCount >= nextTaskUnblockTime) test will pass until
                // there is an item in the delayed list.
                nextTaskUnblockTime = PORT_MAX_DELAY;
            }
            else
            {
                // The new current delayed list is not empty, get the value of
                // the item at the head of the delayed list. This is the item at
                // which the task at the head of the delayed list should be removed
                // from the Blocked state.
                tcb = (TaskControlBlock *)ListGetOwnerOfHeadEntry(delayedTaskList);
                nextTaskUnblockTime = ListGetListItemValue(&(tcb->genericListItem));
            }
        }

        // See if the tick has made a timeout expire.
        CheckDelayedTasks();
    }
    else
    {
        ++missedTicks;

        // The tick hook gets called at regular intervals, even if the
        // scheduler is locked.
        #if(CONFIG_USE_TICK_HOOK == 1)
        {
            ApplicationTickHook();
        }
        #endif
    }

    #if(CONFIG_USE_TICK_HOOK == 1)
    {
        // Guard against the tick hook being called when the missed tick count
        // is being unwound (when the scheduler is being unlocked.)
        if (missedTicks == (unsigned PortBaseType)0U)
        {
            ApplicationTickHook();
        }
    }
    #endif
}

void TaskSwitchContext(void)
{
    if (schedulerSuspended != (unsigned PortBaseType)PD_FALSE)
    {
        // The scheduler is currently suspended
        // - do not allow a context switch.
        missedYield = PD_TRUE;
    }
    else
    {
        TracetaskSwitchedOut();

        // スタックオバーフロー確認
        TaskFirstCheckForStackOverflow();
        TaskSecondCheckForStackOverflow();

        TaskSelectHighestPriorityTask();

        TracetaskSwitchedIn();
    }
}

void TaskPlaceOnEventList(const List * const eventList, PortTickType ticksToWait)
{
    PortTickType timeToWake;

    // THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED OR THE
    // SCHEDULER SUSPENDED. 

    // Place the event list item of the TCB in the appropriate event list.
    // This is placed in the list in priority order so the highest priority task
    // is the first to be woken by the event. 
    ListInsert((List *)eventList, (ListItem *)&(currentTCB->eventListItem));

    // We must remove ourselves from the ready list before adding ourselves
    // to the blocked list as the same list item is used for both lists.  We have
    // exclusive access to the ready lists as the scheduler is locked. 
    if (ListRemove((ListItem *)&(currentTCB->genericListItem)) == 0)
    {

    }

    #if (INCLUDE_TASK_SUSPEND == 1)
    {
        if (ticksToWait == PORT_MAX_DELAY)
        {
            // Add ourselves to the suspended task list instead of a delayed task
            // list to ensure we are not woken by a timing event.  We will block
            // indefinitely. 
            ListInsertEnd((List *)&suspendedTaskList, (ListItem *)&(currentTCB->genericListItem));
        }
        else
        {
            // Calculate the time at which the task should be woken if the event does
            // not occur.  This may overflow but this doesn't matter. 
            timeToWake = tickCount + ticksToWait;
            AddCurrentTaskToDelayedList(timeToWake);
        }
    }
    #else
    {
        // Calculate the time at which the task should be woken if the event does
        // not occur.  This may overflow but this doesn't matter. 
        timeToWake + tickCount + ticksToWait;
        AddCurrentTaskToDelayedList(timeToWake);
    }
    #endif
}

signed PortBaseType TaskRemoveFromEventList(const List * const eventList)
{
    TaskControlBlock *unblockedTCB;
    PortBaseType ret;

    // THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED OR THE
    // SCHEDULER SUSPENDED.  It can also be called from within an ISR.

    // The event list is sorted in priority order, so we can remove the
    // first in the list, remove the TCB from the delayed list, and add
    // it to the ready list.

    // If an event is for a queue that is locked then this function will never
    // get called - the lock count on the queue will get modified instead.  This
    // means we can always expect exclusive access to the event list here.

    // This function assumes that a check has already been made to ensure that
    // eventList is not empty.
    unblockedTCB = (TaskControlBlock *)ListGetOwnerOfHeadEntry(eventList);

    ListRemove(&(unblockedTCB->eventListItem));

    if (schedulerSuspended == (unsigned PortBaseType)PD_FALSE)
    {
        ListRemove(&(unblockedTCB->genericListItem));
        AddTaskToReadyQueue(unblockedTCB);
    }
    else
    {
        // We cannot access the delayed or ready lists, so will hold this
        // task pending until the scheduler is resumed.
        ListInsertEnd((List *)&(pendingReadyList), &(unblockedTCB->eventListItem));
    }

    if (unblockedTCB->priority >= currentTCB->priority)
    {
        // Return true if the task removed from the event list has
        // a higher priority than the calling task.  This allows
        // the calling task to know if it should force a context
        // switch now.
        ret = PD_TRUE;
    }
    else
    {
        ret = PD_FALSE;
    }

    return ret;
}

void TaskSetTimeOutState(TimeOutType * const timeOut)
{
    timeOut->overflowCount = numOfOverflows;
    timeOut->timeOnEntering = tickCount;
}

PortBaseType TaskCheckForTimeOut(TimeOutType * const timeOut, PortTickType * const ticksToWait)
{
    PortBaseType ret;

    TaskEnterCritical();
    {
        #if (INCLUDE_TASK_SUSPEND == 1)
        // If INCLUDE_TASK_SUSPENDED is set to 1 and the block time specified is
        // the maxmum block time then the task should block indefinitely, and
        // therefore never timeout.
        if (*ticksToWait == PORT_MAX_DELAY)
        {
            ret = PD_FALSE;
        }
        // We are not blocking indefinitely, perform the checks below.
        else
        #endif
            if ((numOfOverflows != timeOut->overflowCount)
                && ((PortTickType)tickCount >= (PortTickType)timeOut->timeOnEntering))
            {
                // The tick count is greater than the time at which TaskSetTimeout()
                // was called, but has also overflowed since TaskSetTimeOut() was called.
                // It must have wrapped all the way arrounf and gone past us again. This
                // passed since TaskSetTimeOut() was called.
                ret = PD_TRUE;
            }
            else if (((PortTickType)((PortTickType)tickCount - (PortTickType)timeOut->timeOnEntering)) < (PortTickType)*ticksToWait)
            {
                // Not a genuine timeout. Adjust parameters for time remaining.
                *ticksToWait -= ((PortTickType)tickCount - (PortTickType)timeOut->timeOnEntering);
                TaskSetTimeOutState(timeOut);
                ret = PD_FALSE;
            }
            else
            {
                ret = PD_TRUE;
            }
    }
    TaskExitCritical();

    return ret;
}

void TaskMissedYield(void)
{
    missedYield = PD_TRUE;
}

//
// The Idle Task
//
// The PortTaskFunction() macro is used to allow port/compiler specific
// language extensions.The equivalent prototype for this function is:
//
// void IdleTask(void *parameters);
//
static PortTaskFunction(IdleTask, parameters)
{
    // Stop warnings.
    // コンパイラは使用していない変数について警告を出す.
    (void)parameters;

    for (;;)
    {
        // See if any tasks have been deleted.
        CheckTasksWaitingTermination();

        #if (CONFIG_USE_PREEMPTION == 0)
        {
            // If we are not using preemption we keep forcing a task switch to
            // see if any other task has become available. If we are using
            // preemption we don't need to do this as any task becoming available
            // will automatically get the processor anyway.
            TaskYield();
        }
        #endif

        #if ((CONFIG_USE_PREEMPTION == 1) && (CONFIG_IDLE_SHOULD_YIELD == 1))
        {
            // When using preemption tasks of equal priority will be 
            // timesliced. If a task that is sharing the idle priority is ready
            // to run then the idle task should yield before the end of the
            // timeslice.

            // A critical region is not required here as we are just reading from
            // the list, and an occasinal incorrect value will not matter. If
            // the ready list at the idle priority contains more than one task
            // then a task other than the idle task is ready to execute.
            if (ListCurrentListLength(&(readyTasksLists[IDLE_TASK_PRIORITY])) > (unsigned PortBaseType)1)
            {
                TaskYield();
            }
        }
        #endif

        #if (CONFIG_USE_IDLE_HOOK == 1)
        {
            extern void ApplicationIdleHook(void);

            // Call the user defined function from within the idle task. This
            // allows the application designer to add background functionality
            // whitout the overhead of separate task.
            // NOTE: ApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES, 
            // CALL A FUNCTION THAT MIGHT BLOCK.
            ApplicationIdleHook();
        }
        #endif
    }
}

static void InitialiseTCBVariables(TaskControlBlock *tcb, const signed char * const name,
    unsigned PortBaseType priority, unsigned short stackDepth)
{
    // Store the function name in the TCB.
    #if (CONFIG_MAX_TASK_NAME_LEN > 1)
    {
        // Don't bring strncpy into the build unnecessarily.
        strncpy((char *)tcb->taskName, (const char *)name, (unsigned short)CONFIG_MAX_TASK_NAME_LEN);
    }
    #endif
    tcb->taskName[(unsigned short)CONFIG_MAX_TASK_NAME_LEN - (unsigned short)1] = (unsigned short)'\0';

    // This is used as an array index so must ensure it's not too large.
    // First remove the privilage bit if one is present.
    if (priority >= CONFIG_MAX_PRIORITIES)
    {
        priority = CONFIG_MAX_PRIORITIES - (unsigned PortBaseType) 1U;
    }

    tcb->priority = priority;
    #if (CONFIG_USE_MUTEXES == 1)
    {
        tcb->basePriority = priority;
    }
    #endif

    ListInitialiseItem(&(tcb->genericListItem));
    ListInitialiseItem(&(tcb->eventListItem));

    // Set the tcb as alink back from the ListItem. This is so we can get
    // back to the containing TCB from a generic item in a list.
    ListSetListItemOwner(&(tcb->genericListItem), tcb);

    // Event lists are always in priority order.
    ListSetListItemValue(&(tcb->eventListItem), CONFIG_MAX_PRIORITIES - (PortTickType)priority);
    ListSetListItemOwner(&(tcb->eventListItem), tcb);

}

static void InitialiseTaskLists(void)
{
    unsigned PortBaseType priority;

    for (priority = (unsigned PortBaseType)0U; priority < CONFIG_MAX_PRIORITIES; priority++)
    {
        ListInitialise((List *)&(readyTasksLists[priority]));
    }

    ListInitialise((List *)&delayedTaskList1);
    ListInitialise((List *)&delayedTaskList2);
    ListInitialise((List *)&pendingReadyList);

    #if (INCLUDE_TASK_DELETE == 1)
    {
        ListInitialise((List *)&tasksWaitingTermination);
    }
    #endif

    #if (INCLUDE_TASK_SUSPEND == 1)
    {
        ListInitialise((List *)&suspendedTaskList);
    }
    #endif

    // Start with delayedTaskList using list1 and the overflowDelayedTaskList using list2.
    delayedTaskList = &delayedTaskList1;
    overflowDelayedTaskList = &delayedTaskList2;
}

static void CheckTasksWaitingTermination(void)
{
    #if (INCLUDE_TASK_DELETE == 1)
    {
        PortBaseType listIsEmpty;

        // taskDeleted is used to prevent TaskSuspendAll() being called
        // too often in the idle task.
        while (tasksDeleted > (unsigned PortBaseType)0U)
        {
            TaskSuspendAll();
            {
                listIsEmpty = ListListIsEmpty(&tasksWaitingTermination);
            }
            TaskResumeAll();

            if (listIsEmpty == PD_FALSE)
            {
                TaskControlBlock *tcb;

                TaskEnterCritical();
                {
                    tcb = (TaskControlBlock *)ListGetOwnerOfHeadEntry(((List *)&tasksWaitingTermination));
                    ListRemove(&(tcb->genericListItem));
                    --currentNumberOfTasks;
                    --tasksDeleted;
                }
                TaskExitCritical();

                // ここで, tcbを開放する.
                DeleteTCB(tcb);
            }
        }
    }
    #endif
}

static void AddCurrentTaskToDelayedList(PortTickType timeToWake)
{
    // The list item will be inserted in wake time order.
    ListSetListItemValue(&(currentTCB->genericListItem), timeToWake);

    if (timeToWake < tickCount)
    {
        // Wake time has overflowed. Place this item in overfloaw list.
        ListInsert((List *)overflowDelayedTaskList, (ListItem *)&(currentTCB->genericListItem));
    }
    else
    {
        // The wake time has not overflowd, so we can use the current block list.
        ListInsert((List *)delayedTaskList, (ListItem *)&(currentTCB->genericListItem));

        // If the task entering the blocked state was placed at the head of the 
        // list of blocked tasks the nextTaskUnblockTime needs to be updated too.
        if (timeToWake < nextTaskUnblockTime)
        {
            nextTaskUnblockTime = timeToWake;
        }
    }
}

static TaskControlBlock* AllocateTCBAndStack(unsigned short stackDepth, PortStackType *stackBuffer)
{
    TaskControlBlock *newTCB;

    // Allocate space for the TCB. Where the memory comes from depends on
    // the implementation of the port malloc function.
    newTCB = (TaskControlBlock*)PortMalloc(sizeof(TaskControlBlock));

    if (newTCB != NULL)
    {
        // Allocate space for the stack used by the task being created.
        // The base of the stack memory stored in the TCB sp the task can
        // be deleted later if required.
        newTCB->stack = (PortStackType *)PortMallocAligned((((size_t)stackDepth) * sizeof(PortStackType)), stackBuffer);

        if (newTCB->stack == NULL)
        {
            // Could not allocate the stack. Delete the allocated TCB.
            PortFree(newTCB);
            newTCB = NULL;
        }
        else
        {
            // Just to help debuging.
            memset(newTCB->stack, (int)TASK_STACK_FILL_BYTE, (size_t)stackDepth * sizeof(PortStackType));
        }
    }

    return newTCB;
}

#if (INCLUDE_TASK_DELETE == 1)
static void DeleteTCB(TaskControlBlock *tcb)
{
    // タスクが保有するスタックを開放してから, TCBを開放する.
    // Free up the memory allocated by the scheduler for the task. It is up to
    // the task to free any memory allocated at the application level.
    PortFreeAligned(tcb->stack);
    PortFree(tcb);


}
#endif

#if ((INCLUDE_TASK_GET_CURRENT_TASK_HANDLE == 1) || (CONFIG_USE_MUTEXES == 1))
TaskHandle TaskGetCurrentTaskHandle(void)
{
    TaskHandle ret;

    // A critical section is not required as this is not called from
    // an interrupt and the current TCB will always be the same for any
    // individual execution thread.
    ret = currentTCB;

    return ret;
}
#endif

#if ((INCLUDE_TASK_GET_SCHEDULER_STATE == 1) || (CONFIG_USE_MUTEXES == 1))
PortBaseType TaskGetSchedulerState(void)
{
    PortBaseType ret;

    if (schedulerRunning == PD_FALSE)
    {
        ret = TASK_SCHEDULER_NOT_STARTED;
    }
    else
    {
        if (schedulerSuspended == (unsigned PortBaseType)PD_FALSE)
        {
            ret = TASK_SCHEDULER_RUNNING;
        }
        else
        {
            ret = TASK_SCHEDULER_SUSPENDED;
        }
    }

    return ret;
}
#endif

#if (CONFIG_USE_MUTEXES == 1)
void TaskPriorityInherit(TaskHandle * const mutexHolder)
{
    TaskControlBlock * const tcb = (TaskControlBlock *)mutexHolder;

    // If the mutex was given back by interrupt while the queue was
    // locked then the mutex holder might now be NULL.
    if (mutexHolder != NULL)
    {
        if (tcb->priority < currentTCB->priority)
        {
            // Adjust the mutex holder state to account for its new priority.
            ListSetListItemValue(&(tcb->eventListItem), CONFIG_MAX_PRIORITIES - (PortTickType)currentTCB->priority);

            // If the task being modified is in the ready state it will need to 
            // be moved into a new list.
            if (ListIsContainedWithin(&(readyTasksLists[tcb->priority]), &(tcb->genericListItem)) != PD_FALSE)
            {
                if (ListRemove((ListItem *)&(tcb->genericListItem)) == 0)
                {
                    
                }

                // Inherit the priority before being moved into the new list.
                tcb->priority = currentTCB->priority;
                AddTaskToReadyQueue(tcb);
            }
            else
            {
                // Just inherit the priority.
                tcb->priority = currentTCB->priority;
            }

            TraceTaskPriorityInherit(tcb, currentTCB->priority);
        }
    }

}
#endif

#if (CONFIG_USE_MUTEXES == 1)
void TaskPriorityDisinherit(TaskHandle * const mutexHolder)
{
    TaskControlBlock * const tcb = (TaskControlBlock *)mutexHolder;

    if (mutexHolder != NULL)
    {
        if (tcb->priority != tcb->basePriority)
        {
            // We must be the running task to be able to give the mutex back.
            // Remove ourselves from the ready list we currently appear in.
            if (ListRemove((ListItem *)&(tcb->genericListItem)) == 0)
            {

            }

            // Disinherit the priority before adding the task into the new
            // ready list.
            TraceTaskPriorityDisinherit(tcb, tcb->basePriority);
            tcb->priority = tcb->basePriority;
            ListSetListItemValue(&(tcb->eventListItem), CONFIG_MAX_PRIORITIES - (PortTickType)tcb->priority);
            AddTaskToReadyQueue(tcb);
        }
    }
}
#endif