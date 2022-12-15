#include "../include/clientTCP.h"


int start_con(char *ip, int port, int *sockfd){
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    
    

    /*connect to the server*/
    if (connect(*sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return 0;

}


void get_ip_port(int *port, char *ip, FILE *socket){
    char* fullString = NULL;
    size_t numBytes;

    int foundLine = 0;
    while(!foundLine){
        getline(&fullString, &numBytes, socket);
        printf("# %s \n", fullString);

        if (fullString[3] == ' ') {
            
            foundLine = 1;
        }
    }
    strtok(fullString, "(");

    // Get the IP
    char *token = strtok(NULL, ",");
    strcpy(ip, token);
    for (int i = 0; i < 3; i++){
        token = strtok(NULL, ",");
        strcat(ip, ".");
        strcat(ip, token);
    }
    sprintf(ip, "%s", ip); 

    // Get the port
    token = strtok(NULL, ",");
    *port = atoi(token) * 256;
    token = strtok(NULL, ")");
    *port += atoi(token);
    printf("#Port: %d\n", *port);
    printf("#IP: %s\n\n", ip);
}


//sends a command over a socket connection
int send_command(int socket, char * cmd){
    int bytesSent = write(socket, cmd, strlen(cmd));

    if (bytesSent < 0){
        perror("!Error sending command!");
        return 1;
    }
    
    return 0;
}

int read_reply(FILE * socket){
    char * reply = NULL;
    size_t reply_size = 0;
    long code;
    char * end;

    while(getline(&reply, &reply_size, socket) > 0){
        printf("# %s\n", reply);
        if(reply[3] == ' '){
            code = strtol(reply, &end, 10);
            if(code >= 500 && code <= 559){
                printf("!Error code: %ld!\n", code);
                exit(1);
            }
            break;
        }
    }

    free(reply);
    return 0;
}

int write_file(char *fileN, int socket ){
    FILE *file = fopen(fileN, "w");

    if(file == NULL){
        printf("!Error opening file!\n");
        return 1;
    }

    char *c = malloc(1);
    if (c == NULL)
    {
        printf("!Error allocating memory!\n");
        return 1;
    }
    
    while(read(socket, c, 1) > 0){  // read one byte at a time
        fputc(*c, file);
    }

    free(c);
    fclose(file);
    return 0;
}