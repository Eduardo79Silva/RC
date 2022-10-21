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


int stateMachine(char *buffer, int length, u_int16_t ctrl) // State machine used to check if the received data is a SET or a UA
{
    int currentByte = 0; //Current byte being read, starting at index 0

    while (TRUE)
    {
        switch (state)
        {
        case 0: // Start state
            printf("State 0\n");
            if (buffer[currentByte] == FLAG) // Check if the received byte is a FLAG
            {
                state = 1; // If it is, go to the next state else stay in the same state
            }
            currentByte++; // Read the next byte
            return 0;
        case 1: // FLAG state

            printf("State 1\n");
            if (buffer[currentByte] == A_TX) // Check if the received byte is A_TX
                state = 2;
            else if (buffer[currentByte] != FLAG) // If it isn't, go back to the start state
                state = 0;

            currentByte++;
            return 0;
        case 2: // A state
            printf("State 2\n");
            if (buffer[currentByte] == ctrl) // Check if the received byte is the control byte
                state = 3;
            else if (buffer[currentByte] == FLAG) // If it isn't, but if it is a FLAG go back to the FLAG state
                state = 1;
            else
                state = 0; // If it isn't, go back to the start state

            currentByte++;
            return 0;

        case 3: // C state
            printf("State 3\n");
            if (buffer[currentByte] == buffer[currentByte - 1] ^ buffer[currentByte - 2]) // Check if the received byte is the BCC
                state = 4;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            return 0;

        case 4: // BCC state
            printf("State 4\n");
            if (buffer[currentByte] == FLAG){ // Check if the received byte is a STOP FLAG
                printf("State machine finished\n");
                return TRUE;
            }
            else
                state = 0;

            currentByte++;

        default:
            return -1; // If the state machine is in an invalid state, return -1
        }
    }
}


