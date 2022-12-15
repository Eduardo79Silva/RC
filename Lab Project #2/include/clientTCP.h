#pragma once
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int start_con(char *ip, int port, int *sockfd);
void get_ip_port(int *port, char *ip, FILE *socket);
int send_command(int socket, char * cmd);
int read_reply(FILE * socket);
int write_file(char *file, int socket );

