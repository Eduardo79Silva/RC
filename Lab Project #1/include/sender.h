#include "macros.h"
#include "alarm.h"
#include "state_machine.h"

/* int sendSET(unsigned char *buf);

int senderStart(int fd, int reCount);

int receiveUA(int fd);
 */
void byteStuffing(unsigned char *buffer, int *length);

void byteDestuffing(unsigned char *buffer, int length);