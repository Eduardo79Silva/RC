#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "macros.h"
#include "alarm.h"

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

void disableAlarm(int sig) {
    alarmEnabled = FALSE;
    alarmCount = 0;
    printf("Alarm disabled");
}


int startAlarm(unsigned int time) {
   


    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    while (alarmCount < 4)
    {
        if (alarmEnabled == FALSE)
        {
            alarm(time); 
            alarmEnabled = TRUE;
        }
    }

    printf("Ending program\n");

    return 0;
}
