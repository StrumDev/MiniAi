#include "system/Interrupt.h"
#include <signal.h>

static volatile int requested = 0;

static void InterruptHandler(int number)
{
    (void)number;
    requested = 1;
}

void InterruptArm(void)
{
    requested = 0;
    signal(SIGINT, InterruptHandler);
}

void InterruptDisarm(void)
{
    signal(SIGINT, SIG_DFL);
}

int InterruptRequested(void)
{
    return requested;
}
