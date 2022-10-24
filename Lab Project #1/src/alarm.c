#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "macros.h"
#include "alarm.h"

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal){ // Alarm function handler, fires when the alarm is triggered
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

void disableAlarm() { // Called when UA response is received
    alarmEnabled = FALSE;
    alarmCount = 0;
    printf("Alarm disabled");
}


int startAlarm(unsigned int time) { // Start alarm function
   
    printf("Alarm started\n");

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler); 
    
    if (alarmEnabled == FALSE) { // If the alarm hasn't been started yet
        alarm(time); 
        alarmEnabled = TRUE; 
    }


    return 0;
}
