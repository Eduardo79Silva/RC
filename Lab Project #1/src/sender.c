#include "macros.h"
#include "alarm.h"
#include "state_machine.h"

int localFD;
int nRetransmissions;
int STOPS = 0;


int receiveUA()
{
    unsigned char buf[BUFSIZE] = {0};

        int bytes = read(localFD, buf, 5);
        printf("Received %d \n", localFD);
    while(STOPS == FALSE){
        //printf("Entered while\n");
        if (buf != 0 && bytes > -1)
        {
            printf("Received %02x \n", buf[0]);
            int ans = stateMachine(buf, LlTx); // Check if the received data is a UA response
            printf("Answer: %d\n", ans);
            if (ans == 1)
            {
                printf("\nUA received\n");
                disableAlarm(); // If it is, disable the alarm
                STOPS = TRUE;
                return 1;
            }
        }
    }
    return 0;
}

int sendSET(unsigned char *buf)
{
    buf[0] = FLAG; // FLAG
    buf[1] = A_TX; // A
    buf[2] = C_SET; // C
    buf[3] = BCC(buf[1], buf[2]); // BCC
    buf[4] = FLAG; // FLAG

    int bytes = write(localFD, buf, 5);
    printf("SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

int senderStart(int fd, int reCount) 
{
    localFD = fd;
    nRetransmissions = reCount; // Number of retransmissions
    unsigned char buf[BUFSIZE] = {0}; // Buffer to store the received data

    while(nRetransmissions > 0){ // While there are retransmissions left and no UA flag has been received
        
        if(!alarmEnabled){ //If the alarm hasn't been started yet
            sendSET(&buf); //Send SET command to the receiver
            startAlarm(5); //Start alarm to wait for UA response
            nRetransmissions--; //Decrement the number of retransmissions
        }

        if(receiveUA()) return 1; // If UA flag has been received, return 1 to indicate success and exit the function
    }
    

    return 0;
    
}





// void byteDestuffing(unsigned char *buffer, int length)
// {
//     int i = 0;
//     int j = 0;
//     int k = 0;
//     int flag = 0;
//     unsigned char *temp = (unsigned char *)malloc(length * sizeof(unsigned char));

//     for (i = 0; i < length; i++)
//     {
//         if (buffer[i] == 0x7D)
//         {
//             if (buffer[i + 1] == 0x5E)
//             {
//                 temp[j] = 0x7E;
//                 i++;
//                 j++;
//             }
//             else if (buffer[i + 1] == 0x5D)
//             {
//                 temp[j] = 0x7D;
//                 i++;
//                 j++;
//             }
//         }
//         else
//         {
//             temp[j] = buffer[i];
//             j++;
//         }
//     }

//     for (k = 0; k < j; k++)
//     {
//         buffer[k] = temp[k];
//     }
// }

// void byteStuffing(char *buffer, int length, char *buffer2)
// {
//     int currentByte = 0;
//     int currentByte2 = 0;

//     while (currentByte < length)
//     {
//         if (buffer[currentByte] == FLAG)
//         {
//             buffer2[currentByte2] = ESCAPE;
//             currentByte2++;
//             buffer2[currentByte2] = FLAG ^ 0x20;
//             currentByte2++;
//         }
//         else if (buffer[currentByte] == ESCAPE)
//         {
//             buffer2[currentByte2] = ESCAPE;
//             currentByte2++;
//             buffer2[currentByte2] = ESCAPE ^ 0x20;
//             currentByte2++;
//         }
//         else
//         {
//             buffer2[currentByte2] = buffer[currentByte];
//             currentByte2++;
//         }
//         currentByte++;
//     }
// }

