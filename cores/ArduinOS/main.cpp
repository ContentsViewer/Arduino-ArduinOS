#include <Arduino.h>

unsigned PortBaseType mainSetupPriority = HIGH_PRIORITY;
unsigned short mainSetupStackSize = CONFIG_MINIMAL_STACK_SIZE;
unsigned PortBaseType mainLoopPriority = LOW_PRIORITY;
unsigned short mainLoopStackSize = CONFIG_MINIMAL_STACK_SIZE;

TaskHandle loopTaskHandle;
TaskHandle setupTaskhandle;


void MainTask(void *parameters)
{
    for (;;)
    {
        loop();
        if (serialEventRun) serialEventRun();
    }
}


void SetupTask(void *parameters)
{
    // SetupTaskが処理を終えるまで他タスクを実行させない.
    TaskSuspendAll();
    {
        setup();
        TaskCreate(MainTask, (signed PortChar *)"Main", mainLoopStackSize, NULL, mainLoopPriority, &loopTaskHandle);
    }
    TaskResumeAll();

    #if INCLUDE_TASK_DELETE
        TaskDelete(NULL);
    #else
        while (true) TaskSuspend(NULL);
    #endif
}


int main()
{
    init();

#if defined(USBCON)
    USBDevice.attach();
#endif

    TaskCreate(SetupTask, (signed PortChar *)"Setup", mainSetupStackSize, NULL, mainSetupPriority, &setupTaskhandle);
    //setup();
    //TaskCreate(MainTask, (signed PortChar *)"Main", mainLoopStackSize, NULL, mainLoopPriority, &loopTaskHandle);

    TaskStartScheduler();

    for (;;);

    return 0;
}