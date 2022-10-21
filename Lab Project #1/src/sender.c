#include "macros.h"

int sendSET(int fd, unsigned char *msg)
{
    msg[0] = 0x7E; // FLAG
    msg[1] = 0x03; // A
    msg[2] = 0x03; // C
    msg[3] = 0x00; // BCC
    msg[4] = 0x7E; // FLAG

    int bytes = write(fd, msg, 5);
    printf("SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

int senderStart(int fd)
{
    unsigned char buf[BUFSIZE] = {0};

    sendSET(fd, &buf);

    return 1;
    
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

