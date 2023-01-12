/*reg no- MT2022112 name - Shubhanshi Bhandari
client side program for both a and b part of 34


*/


#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/ip.h> 
#include <stdio.h>      
#include <unistd.h>     
#include<fcntl.h>
#include<sys/stat.h>
#include <arpa/inet.h>
#include<netdb.h>
#include<string.h>
#include<errno.h>
#include<stdbool.h>
void connection_handler(int sockFD); 
struct message{
char writeBuffer[1000];
bool response;
};

void main()
{
    int sfd;     
    int cstatus;
    struct sockaddr_in address; 
    int x=2;
    char buff[100];
    //printf("\033[107m");
        sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd== -1)
    {
        perror("Error while creating socket!");
        _exit(0);
    }
    printf("Client side socket successfully created!\n");

        

    address.sin_addr.s_addr = inet_addr("172.16.144.70");
    address.sin_family = AF_INET;
    address.sin_port = htons(5500);
   
    cstatus=connect(sfd, (struct sockaddr *)&address, sizeof(address));
    if (cstatus == -1)
    {
        perror("Error while connecting to server!");
        close(sfd);
        _exit(0);
    }
    printf("Client to server connection successfully established!\n");
    printf("\t\tWelcome to Anuradha Laxmi bank\n");
    connection_handler(sfd);
     close(sfd);
}
void connection_handler(int sockFD)
{
    char writeBuffer[1000]; 
    struct message msg;
    ssize_t readBytes, writeBytes;            
    char tempBuffer[1000];

    do
    {
       
        memset(writeBuffer, 0,sizeof(writeBuffer));
   
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        readBytes = read(sockFD,&msg, sizeof(msg));
        if (readBytes == -1)
            perror("Error while reading from client socket!");
        else if (readBytes == 0)
            printf("No msg from bank reopen this program\n");
       
 
        
        else{
        printf("%s",msg.writeBuffer);
         if(msg.response==1){
         scanf(" %[^\n]", writeBuffer);
        writeBytes = write(sockFD, writeBuffer, strlen(writeBuffer)); 
    }
    }
     }while (readBytes > 0);

    close(sockFD);
}













