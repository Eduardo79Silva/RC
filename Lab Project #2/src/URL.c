#include "../include/URL.h"

int get_ip(char *host, URL *url_struct){
    struct hostent *h;

    if ((h = gethostbyname(host)) == NULL){
        herror("gethostbyname()");
        return 1;
    }

    printf("IP Address >>> %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));

    strcpy(url_struct->ip,inet_ntoa(*((struct in_addr *) h->h_addr)));

    return 0;
}



int parse_url(char *url, URL *url_struct){
    char fullpath[256];
    char* token;

    char* ftp = strtok(url, "/");  
    char* urlrest = strtok(NULL, "/"); 
    char* path = strtok(NULL, "");      

    if (strcmp(ftp, "ftp:") != 0){
        printf("!Uh Oh, not using ftp!\n");
        return 1;
    }

    char* user = strtok(urlrest, ":");
    char* pass = strtok(NULL, "@");

    // no user:password given
    if (pass == NULL)
    {
        user = "anonymous";
        pass = "pass";
        strcpy(url_struct->host, urlrest);
    } else
        strcpy(url_struct->host, strtok(NULL, ""));

    strcpy(url_struct->path, path);
    strcpy(url_struct->user, user);
    strcpy(url_struct->password, pass);

    if(get_ip(url_struct->host,url_struct) != 0){
        printf("!Error getting host name!\n");
        return 1;
    }

    // extract the file name from the path
    strcpy(fullpath, url_struct->path);
    char* token2 = strtok(fullpath, "/");
    while( token2 != NULL ) {
        strcpy(url_struct->filename, token2);
        token2 = strtok(NULL, "/");
    }

    return 0;
}


