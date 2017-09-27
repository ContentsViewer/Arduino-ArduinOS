#include <Arduino.h>


unsigned PortBaseType mainSetupPriority = HIGH_PRIORITY;
unsigned short mainSetupStackSize = CONFIG_MINIMAL_STACK_SIZE;  //‘O‚Í‚±‚ê‚É5”{‚µ‚Ä‚¢‚½
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
    setup();

    TaskCreate(MainTask, (signed PortChar *)"Main", mainLoopStackSize, NULL, mainLoopPriority, &loopTaskHandle);

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

    TaskStartScheduler();

    for (;;);

    return 0;
}