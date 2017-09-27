#ifndef MEGA_OS_CONFIG_ATMEGA2560_H
#define MEGA_OS_CONFIG_ATMEGA2560_H

// Memory
#define CONFIG_MINIMAL_STACK_SIZE ((unsigned PortShort) 85)
#define CONFIG_TOTAL_HEAP_SIZE ((size_t) (4096))

// Timer
#define CONFIG_USE_16_BIT_TICKS 1

// Task
#define CONFIG_MAX_TASK_NAME_LEN (16)

// Mutex
#define CONFIG_USE_MUTEXES 1

// -------------------------------------------------------------------------
// Set the following definitions to 1 to include the API function, or zero
// to wxclude the API function
#define INCLUDE_TASK_DELETE 1
#define INCLUDE_TASK_SUSPEND 1
#define INCLUDE_TASK_DELAY 1
#define INCLUDE_TASK_DELAY_UNTIL 1
#define INCLUDE_TASK_GET_SCHEDULER_STATE 0

#endif
