#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "macros.h"

int state = 0; // 0 = Start; 1 = FLAG; 2 = A; 3 = C; 4 = BCC; 5 = STOP


int stateMachine(char *buffer, int length, u_int16_t ctrl)
{
    int currentByte = 0;

    while (TRUE)
    {
        switch (state)
        {
        case 0:
            printf("State 0\n");
            if (buffer[currentByte] == FLAG)
            {
                state = 1;
            }
            currentByte++;
            return 0;
        case 1:
            printf("State 1\n");
            if (buffer[currentByte] == A_TX)
                state = 2;
            else if (buffer[currentByte] != FLAG)
                state = 0;

            currentByte++;
            return 0;
        case 2:
            printf("State 2\n");
            if (buffer[currentByte] == ctrl)
                state = 3;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            return 0;

        case 3:
            printf("State 3\n");
            if (buffer[currentByte] == buffer[currentByte - 1] ^ buffer[currentByte - 2])
                state = 4;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            return 0;

        case 4:
            printf("State 4\n");
            if (buffer[currentByte] == FLAG){
                printf("State machine finished\n");
                return TRUE;
            }
            else
                state = 0;

            currentByte++;

        default:
            return -1;
        }
    }
}


