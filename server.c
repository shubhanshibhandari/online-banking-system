#include <stdio.h> 
#include <errno.h> 

#include <fcntl.h> 
#include <unistd.h>    
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/ip.h> 

#include <string.h>  
#include <stdbool.h> 
#include <stdlib.h>  


#include "./functions/admin.h"
#include "./functions/customer.h"

void connection_handler(int connFD); 

void main()
{
    int sfd, socketBindStatus, socketListenStatus, connfd;
    struct sockaddr_in server, client;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perror("Error while creating server socket!");
        _exit(0);
    }

    server.sin_family = AF_INET;                
    server.sin_port = htons(5500);              
    server.sin_addr.s_addr = htonl(INADDR_ANY); 

    bind(sfd, (struct sockaddr *)&server, sizeof(server));
  

     listen(sfd, 10);
    //rm
  
    printf("Now listening for connections on a socket!\n");
    int sz;
    while (1)
    {
        sz = (int)sizeof(client);
        connfd = accept(sfd, (struct sockaddr *)&client, &sz);
        //rm
        if (connfd == -1)
        {
            perror("Error while connecting to client!");
            close(sfd);
        }
        else
        {
            if (!fork())
            {
                // Child will enter this branch
                connection_handler(connfd);
                close(connfd);
                _exit(0);
            }
        }
    }

    close(sfd);
}

void connection_handler(int connfd)
{
    printf("Client has connected to the server!\n");

    char readBuffer[1000];//writeBuffer[1000];
    struct message msg;
    ssize_t readBytes, writeBytes;
    int ch;

       while(1){
       memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"user type:\n1. Admin\t2. Customer\nPress any other number to exit\n");
        msg.response=1;
        write(connfd, &msg, sizeof(msg));
   
      
        memset(readBuffer, 0,sizeof(readBuffer));
        readBytes = read(connfd, readBuffer, sizeof(readBuffer));
        //rm
        if (readBytes == -1)
            perror("Error while reading from client");
        else if (readBytes == 0)
            printf("No data was sent by the client");
        
        else
        {   
            ch = atoi(readBuffer);
            switch (ch)
            {
            case 1:
                // Admin
                admin_operation_handler(connfd);
                break;
            case 2:
                // Customer
                customer_operation_handler(connfd);
                break;
            default:
                // Exit
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"invalid choice.\nreopen the application\n" );
        msg.response=0;
        write(connfd, &msg, sizeof(msg));
                break;
            }
        }
    }
    printf("Terminating connection to client!\n");
}
