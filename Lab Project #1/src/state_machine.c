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




void stateMachine(char *buffer, int* state, unsigned int C) // State machine used to check if the received data is a SET or a UA
{

    switch (*state){
        case START: // Start state
            printf("State 0\n");
            if (*buffer == FLAG){ // Check if the received byte is a FLAG
                printf("FLAG received\n");
                *state = FLAG_RCV; // If it is, go to the next state else stay in the same state
            }
            break;

        case FLAG_RCV: // FLAG state

            printf("State 1\n");
            if (*buffer == A_TX) // Check if the received byte is A_TX
                *state = A_RCV;
            else if (*buffer != FLAG) // If it isn't, go back to the start state
                *state = FLAG_RCV;

            break;

        case A_RCV: // A state
/*             printf("State 2\n");
            printf("current: %d\n", buffer[0]);
            printf("C: %d\n", buffer[1]);
            printf("A: %d\n", buffer[2]);
            printf("A: %d\n", buffer[3]);           
            printf("A: %d\n", buffer[4]); */
            if (*buffer == C) // Check if the received byte is the control byte
                *state = C_RCV;
            else if (*buffer == FLAG) // If it isn't, but if it is a FLAG go back to the FLAG state
                *state = FLAG_RCV;
            else
                *state = START; // If it isn't, go back to the start state

            break;


        case C_RCV: // C state
            printf("State 3\n");
            if (*buffer ==  BCC(A_TX,C)) // Check if the received byte is the BCC
                *state = BCC_OK;
            else if (*buffer == FLAG)
                *state = FLAG_RCV;
            else
                *state = START;

            break;


        case BCC_OK: // BCC state
            printf("State 4\n");
            if (*buffer == FLAG){ // Check if the received byte is a STOP FLAG
                printf("State machine finished\n");
                *state = STOP_ST;
            }
            else{
                *state = START;
            }
            break;

    }

}


