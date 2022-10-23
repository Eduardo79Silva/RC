#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "macros.h"
#include "state_machine.h"




STATE state = START; // 0 = Start; 1 = FLAG; 2 = A; 3 = C; 4 = BCC; 5 = STOP

STATE stateMachine(char *buffer, LinkLayerRole role) // State machine used to check if the received data is a SET or a UA
{
    int currentByte = 0; //Current byte being read, starting at index 0
    u_int16_t ctrl;

    if(role == LlTx){ //If the role is transmitter
        ctrl = C_UA; //The control field should be UA
    }
    else {
        ctrl = C_SET; //The control field should be SET
    }

    while (TRUE){
        switch (state)
        {
        case START: // Start state
            printf("State 0\n");
            if (buffer[currentByte] == FLAG) // Check if the received byte is a FLAG
            {
                printf("FLAG received\n");
                state = FLAG_RCV; // If it is, go to the next state else stay in the same state
            }
            currentByte++; // Read the next byte
            return START;
        case FLAG_RCV: // FLAG state

            printf("State 1\n");
            if (buffer[currentByte] == A_TX) // Check if the received byte is A_TX
                state = A_RCV;
            else if (buffer[currentByte] != FLAG) // If it isn't, go back to the start state
                state = FLAG_RCV;

            currentByte++;
            return START;
        case A_RCV: // A state
            printf("State 2\n");
            if (buffer[currentByte] == ctrl) // Check if the received byte is the control byte
                state = C_RCV;
            else if (buffer[currentByte] == FLAG) // If it isn't, but if it is a FLAG go back to the FLAG state
                state = FLAG_RCV;
            else
                state = START; // If it isn't, go back to the start state

            currentByte++;
            return START;

        case C_RCV: // C state
            printf("State 3\n");
            if (buffer[currentByte] == buffer[currentByte - 1] ^ buffer[currentByte - 2]) // Check if the received byte is the BCC
                state = BCC_OK;
            else if (buffer[currentByte] == FLAG)
                state = FLAG_RCV;
            else
                state = START;

            currentByte++;
            return START;

        case BCC_OK: // BCC state
            printf("State 4\n");
            if (buffer[currentByte] == FLAG){ // Check if the received byte is a STOP FLAG
                printf("State machine finished\n");
                return FLAG_RCV;
            }
            else
                state = START;

            currentByte++;

        default:
            return INVALID; // If the state machine is in an invalid state, return -1
        }
    }
}


