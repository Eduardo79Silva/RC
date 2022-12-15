#pragma once
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include <string.h>

// URL structure
typedef struct URL{
    char user[256];
    char password [256];
    char host[256];
    char path[256];
    char filename[256];
    char ip[256];
    int port;
} URL;


int get_ip(char *host, URL *url_struct);
int parse_url(char *url, URL *url_struct);