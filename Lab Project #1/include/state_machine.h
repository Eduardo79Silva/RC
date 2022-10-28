#pragma once
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "link_layer.h"

void stateMachine(unsigned char *buffer, STATE* state, unsigned int C);

int dataStateMachine(unsigned char *frame, STATE* st, unsigned char *cmdFrame, unsigned char *reading, unsigned int *sizeInfo);