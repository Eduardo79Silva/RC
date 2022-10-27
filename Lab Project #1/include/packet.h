#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "link_layer.h"
#include "macros.h"
#include "string.h"
#include <math.h>
#include <sys/stat.h>


int getControlPacket(char* filename, int start, unsigned char* packet);

int getDataPacket(unsigned char* bytes, unsigned char* packet, int nSequence, int nBytes);