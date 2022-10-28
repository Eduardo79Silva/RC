#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <alarm.h>
#include <unistd.h>
#include "macros.h"
#include "state_machine.h"




void stateMachine(unsigned char *buffer, STATE* state, unsigned int C) // State machine used to check if the received data is a SET or a UA
{

    switch (*state){
        case START: // Start state
            //printf("State 0\n");
            if (*buffer == FLAG){ // Check if the received byte is a FLAG
    //printf("FLAG received\n");
                *state = FLAG_RCV; // If it is, go to the next state else stay in the same state
            }
            break;

        case FLAG_RCV: // FLAG state

           // printf("State 1\n");
            if (*buffer == A_TX) // Check if the received byte is A_TX
                *state = A_RCV;
            else if (*buffer != FLAG) // If it isn't, go back to the start state
                *state = START;

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
            //printf("State 3\n");
            if (*buffer ==  BCC(A_TX,C)) // Check if the received byte is the BCC
                *state = BCC_OK;
            else if (*buffer == FLAG)
                *state = FLAG_RCV;
            else
                *state = START;

            break;


        case BCC_OK: // BCC state
            //printf("State 4\n");
            if (*buffer == FLAG){ // Check if the received byte is a STOP FLAG
                *state = STOP_ST;
            }
            else{
                *state = START;
            }
            break;

    }

}

int dataStateMachine(unsigned char *frame, STATE* st, unsigned char *cmdFrame, unsigned char *reading, unsigned int *sizeInfo){
    unsigned char STOP = FALSE;
    switch (*st)
        {
        case START:
            if(cmdFrame[0] == FLAG){
                *st = FLAG_RCV;
                frame[(*sizeInfo)++] = cmdFrame[0];
            }
            break;

        case FLAG_RCV:
            if(cmdFrame[0] != FLAG){
            
                *st = A_RCV;
                frame[(*sizeInfo)++] = cmdFrame[0];
            }
            else{
                memset(frame, 0, 600);
                *st = FLAG_RCV;
                sizeInfo = 0;
                frame[(*sizeInfo)++] = cmdFrame[0];
            }
            break;

        case A_RCV:
             if(cmdFrame[0] != FLAG){
                frame[(*sizeInfo)++] = cmdFrame[0];
            }
            else if(cmdFrame[0] == FLAG){
            
                STOP = TRUE;
                frame[(*sizeInfo)++] = cmdFrame[0];
                reading = FALSE;
            }
            break;
        
        default:
            break;
        }

        return STOP;
}

int closeStateMachine(unsigned char *cmdFrame, unsigned char *responses, int result, int fd){

    if(result != -1 && responses != 0 && responses[0]==FLAG){
                if(strcasecmp(cmdFrame, responses) != 0){
                    printf("\n#     Wrong DISC: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                    alarmEnabled = FALSE;
                    return -1;
                }
                
                else{   
                    printf("\n#     Correct DISC: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                    alarmEnabled = FALSE;
                    
                    cmdFrame[1] = 1;
                    cmdFrame[2] = 0x07;
                    cmdFrame[3] = BCC(cmdFrame[1],cmdFrame[2]);

                    int frameBytes = write(fd, cmdFrame, 5);

                    close(fd);

                    printf("\n#     Sent UA, %d bytes written.\n\nWe will miss you Tux! <3\n", frameBytes);
                    return 1;

                }
            }

    return 0;
}
