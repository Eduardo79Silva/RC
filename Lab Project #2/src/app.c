#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "../include/URL.h"
#include "../include/clientTCP.h"

int main(int argc, char **argv) {

    if(argc != 2){
        exit(1);
    }

    struct URL urlStruct;
    int socketfd, socket;

    printf("URL >>> %s\n", argv[1]);

	// Parse the URL and check for errors
    if(parse_url(argv[1], &urlStruct) != 0){
        printf("!Error parsing input!\n");
        return 1;
    }
    
	// Start the connection and check for errors 
    if(start_con(urlStruct.ip, 21, &socketfd) != 0){ // check port config
        printf("!Error starting connection!\n");
        return 1;
    }

	// Open the socket and check for errors
    FILE * read_socket = fdopen(socketfd, "r");
	// Read the initial reply from the FTP server
    if (read_reply(read_socket) != 0){
		return 1;
	}
    
    
    char cmd[263];
	// Send the username to the FTP server 
    sprintf(cmd, "user %s\n",urlStruct.user);
    if (send_command(socketfd,cmd) != 0){
		return 1;
	}
    if (read_reply(read_socket) != 0){
		return 1;
	}

    // Send the password to the FTP server
    sprintf(cmd, "pass %s\n",urlStruct.password);
    if (send_command(socketfd,cmd) != 0){
		return 1;
	}
	if (read_reply(read_socket) != 0){
		return 1;
	}
    
    //  Send the PASV command to the FTP server 
    sprintf(cmd, "pasv \n");
    if (send_command(socketfd,cmd) != 0){
		return 1;
	}

    //  Get the port and IP from the PASV reply
    char ip[32];
    int port;
    get_ip_port(&port, ip, read_socket);
	

    // Connect to the data port and wait for the reply
    if(start_con(ip, port, &socket) != 0){
        printf("!Error starting connection!\n");
        return 1;
    }

    // Send the RETR command to the FTP server 

    sprintf(cmd, "retr %s\r\n",urlStruct.path); // retrieve file from path
    if(send_command(socketfd,cmd) != 0){
		return 1;
	}
    if(read_reply(read_socket) != 0){
		return 1;
	}

	// Write the contents of the file to a local file with the same name
    if(write_file(urlStruct.filename, socket) != 0){
		return 1;
	}

    // Quit the FTP session and close the connection
    sprintf(cmd, "quit \r\n");
    if(send_command(socketfd,cmd) != 0){
		return 1;
	}
    
    return 0;

}