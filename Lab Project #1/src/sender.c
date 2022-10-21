#include "macros.h"
#include "alarm.h"

int localFD;
int nRetransmissions;



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
    nRetransmissions = reCount;
    unsigned char buf[BUFSIZE] = {0};

    while(nRetransmissions > 0){
        
        if(!alarmEnabled){
        sendSET(&buf);
        startAlarm(5);
        nRetransmissions--;
    }

        if(receiveUA()) return 1;
    }
    

    return 0;
    
}

int receiveUA()
{
    unsigned char buf[BUFSIZE] = {0};

    int bytes = read(localFD, buf, 5);
    if (buf != 0 && bytes > -1)
    {
        printf("Received %02x \n", buf[0]);
        int ans = stateMachine(buf, BUFSIZE, C_UA);
            printf("\nUA received\n");
            disableAlarm();
        if (ans == 1)
        {
            return 1;
        }
        
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

