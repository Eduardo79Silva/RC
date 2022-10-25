#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "macros.h"
#include "utils.h"
#include "alarm.h"
#include "state_machine.h"

extern int alarmEnabled;

unsigned char BCC2creator(const unsigned char *buf,int bufSize){
    unsigned char BCC2 = 0x00;
    BCC2 = buf[0];

    for(int i = 1; i <= bufSize-1; i++){
        BCC2 ^= buf[i];
    }
    return BCC2;

}

int readCtrlMessage(unsigned char *buffer, int* state, unsigned int C){
    //unsigned char c;
    unsigned char ctrl_camp = NULL;

    //stateMachine(&c,&state,&ctrl_camp)
    
    //ESTE SWITCH É IGUAL À STATE MACHINE APENAS TEM O A_RCV DIFERENTE, PODEMOS ADICIONAR MAS TERÍAMOS QUE COMEÇAR A PASSAR O ATRIBUTO "C" POR REFERENCIA
    switch (*state){

    case START:
        if(*buffer == FLAG){
            *state = FLAG_RCV;
        }
        break;

    case FLAG_RCV:
        if(*buffer == A_TX){
            *state = A_RCV;
        }
        else if (*buffer != FLAG){
            *state = START;
        }
        break;
    case A_RCV:
        if(*buffer == C_RR0 || *buffer == C_RR1 || *buffer == DISC){
            ctrl_camp = *buffer;
            *state = C_RCV;
        }
        else if(*buffer == C_REJ0 || *buffer == C_REJ1){
            //NESTE CASO EU NÃO SEI O QUE FAZER, HELPPPPPPPPPPPPP!!!!!!!!!!!!!!!!     <------------------------------------------------
            ctrl_camp = *buffer;
            *state = START;
            return -1;
        }
        else if(*buffer == FLAG){
            *state = FLAG_RCV;
        }
        else{
            *state = START;
        }
    case C_RCV:
        if(*buffer == BCC(A_TX,ctrl_camp)){
            *state = BCC_OK;
        }
        else if (*buffer == FLAG){
            *state = FLAG_RCV;
        }
        else{
            *state = START;
        }
    case BCC_OK:
        if(*buffer == FLAG){
            *state = STOP_ST;
        }
        else{
            *state = START;
        }
    }

        

    //disableAlarm();

    return ctrl_camp;
}
