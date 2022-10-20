#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "macros.h"

int stateMachine(char *buffer, int length, u_int16_t ctrl)
{
    int currentByte = 0;

    int state = 0; // 0 = Start; 1 = FLAG; 2 = A; 3 = C; 4 = BCC; 5 = STOP

    while (currentByte < length)
    {
        switch (state)
        {
        case 0:
            if (buffer[currentByte] == FLAG)
            {
                state = 1;
            }
            currentByte++;
            break;
        case 1:
            if (buffer[currentByte] == A_TX)
                state = 2;
            else if (buffer[currentByte] != FLAG)
                state = 0;

            currentByte++;
            break;
        case 2:
            if (buffer[currentByte] == ctrl)
                state = 3;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            break;

        case 3:
            if (buffer[currentByte] == buffer[currentByte - 1] ^ buffer[currentByte - 2])
                state = 4;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            break;

        case 4:
            if (buffer[currentByte] == FLAG)
                return TRUE;
            else
                state = 0;

            currentByte++;
            break;

        default:
            break;
        }
    }
}


