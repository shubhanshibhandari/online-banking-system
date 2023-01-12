

#include <stdio.h>     
#include <unistd.h>    
#include <string.h>    
#include <stdbool.h>   
#include <sys/types.h> 
#include <sys/stat.h>  
#include <fcntl.h>     
#include <stdlib.h>    
#include <errno.h>     

#include "../struct/account.h"
#include "../struct/customer.h"
#include "../struct/transaction.h"
#include "./admin_cred.h"
#include <sys/ipc.h>
#include <sys/sem.h>
union semun{
 int val;
 struct semid_ds *buf;
 unsigned short int *array;

};
union semun arg3;
int semid_customer;
struct message{
char writeBuffer[1000];
bool response;
};
struct sembuf semOp3;
bool login_handler(bool isAdmin, int connFD, struct Customer *ptrToCustomer);
bool get_account_details(int connFD, struct Account *customerAccount);
bool get_customer_details(int connFD, int customerID);
bool get_transaction_details(int connFD, int accountNumber);

bool login_handler(bool isAdmin, int connFD, struct Customer *ptrToCustomerID)
{    
    ssize_t readBytes, writeBytes;            
    char readBuffer[1000];
    //char writeBuffer[1000]; 
    struct message msg;
    char tempBuffer[1000];
    struct Customer customer;

    int ID;
// login customer id semaphore

        

    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    memset(readBuffer, 0,sizeof(readBuffer));
    
    
        
        
        strcpy(msg.writeBuffer, "please enter credential for login:\nEnter login ID\n");
        msg.response=1;
 

    //writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
   write(connFD, &msg, sizeof(msg));

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
   

    bool userFound = false;

    if (isAdmin)
    {
        if (strcmp(readBuffer,ADMIN_LOGIN_ID) == 0)
            userFound = true;
    }
    else
    {
      
        memset(tempBuffer, 0,sizeof(tempBuffer));
        
        strcpy(tempBuffer, readBuffer);
       
        int i=0;
	while(readBuffer[i]!='_'){
	    i++;
	}

       for(int j=i+1,k=0;j<strlen(readBuffer);j++,k++){
             tempBuffer[k]=readBuffer[j];
   }
    
    int ID=atoi(tempBuffer);
  
        int custfd = open("./records/customer.bank", O_RDONLY);

        if (custfd == -1 || ID==0)
        {   memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer,"invalid action\n.Reopen application\n" );
            msg.response=0;
            write(connFD, &msg, sizeof(msg));
            
            
            perror("Error opening customer file in read mode!");
            
            return false;
        }
        //offset check
        off_t end=lseek(custfd,0,SEEK_END);
        off_t offset = lseek(custfd, (ID-1) * sizeof(struct Customer), SEEK_SET);
        if (offset >= 0 && end > offset)
        {
           
            struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Customer); 
	readlock.l_pid = getpid(); 
	
        
        int lockingStatus = fcntl(custfd, F_SETLKW, &readlock);
            
           
            readBytes = read(custfd, &customer, sizeof(struct Customer));
           
            readlock.l_type = F_UNLCK;
            fcntl(custfd, F_SETLK, &readlock);
            
            key_t key3 = ftok("./records/customer.bank", customer.id); 

       

        int semctlStatus;
        semid_customer = semget(key3, 1, 0); 
        if (semid_customer == -1)
        {
            semid_customer = semget(key3, 1, IPC_CREAT | 0744); 
            //rm
            if (semid_customer == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            arg3.val = 1; 
            semctlStatus = semctl(semid_customer, 0, SETVAL, arg3);
            
            //rm
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }
                    
               
               
               

            if (strcmp(customer.login, readBuffer) == 0)
             {   if ((semctl(semid_customer,0,GETVAL))<=0 )
                { strcpy(msg.writeBuffer, "user already logged in some other device.\nReopen the application.\n");
            msg.response=0;
            
            write(connFD, &msg, sizeof(msg));
                }
                
                else{
                semOp3.sem_num = 0;
                semOp3.sem_flg = SEM_UNDO;
                semOp3.sem_op = -1;
                semop(semid_customer, &semOp3, 1); 
                
                userFound = true;
           }
            
        }
        close(custfd);
        }
        else
        {   strcpy(msg.writeBuffer, "invalid login id.\nReopen the application.\n");
            msg.response=0;
           
            write(connFD, &msg, sizeof(msg));
        }
    }

    if (userFound)
    {
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
      
        strcpy(msg.writeBuffer,"enter your password:\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        
        memset(readBuffer, 0,sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        
        if (isAdmin)
        {
            if (strcmp(readBuffer, ADMIN_PASSWORD) == 0)
                return true;
        }
        else
        {
            if (strcmp(readBuffer, customer.password) == 0)
            {
                *ptrToCustomerID = customer;
                return true;
            }
        }

        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
       
        strcpy(msg.writeBuffer,"invalid password for the given id\n.Reopen the applicaton\n" );
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        semOp3.sem_op = 1;
        semop(semid_customer, &semOp3, 1);
    }
    else
    {
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
     
        strcpy(msg.writeBuffer,"invalid log in .Reopen application\n" );
        msg.response=0;
        write(connFD, &msg, sizeof(msg));   
    }
    
    return false;
}

bool get_account_details(int connFD, struct Account *customerAccount)
{
    ssize_t readBytes, writeBytes;            
    char readBuffer[1000]; //writeBuffer[1000]; 
    struct message msg;
        
    
    char tempBuffer[1000];

    int accountNumber;
    struct Account account;
    int accfd;

    if (customerAccount == NULL)
    {

       
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "enter account no:\n");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        memset(readBuffer, 0,sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
       

        accountNumber = atoi(readBuffer);
    }
    else
        accountNumber = customerAccount->accountNumber;

    accfd = open("./records/account.bank", O_RDONLY);
    if (accfd == -1)
    {
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"invalid account no\n");
        msg.response=0;
        perror("Error opening account file");
        write(connFD, &msg, sizeof(msg));
        
        
        
      
        return false;
    }
    //offset check
    off_t end=lseek(accfd,0,SEEK_END);
    int offset = lseek(accfd, (accountNumber-54000) * sizeof(struct Account), SEEK_SET);
    if ((offset == -1 && errno == EINVAL) || end<=offset)
    {
        
        
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "invalid account no:\n");
        msg.response=0;
        
        perror("Error seeking to account record in get_account_details!");
       
        write(connFD, &msg, sizeof(msg));
        
        
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required account record!");
        return false;
    }

    struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Account); 
	readlock.l_pid = getpid(); 
        int lockingStatus = fcntl(accfd, F_SETLKW, &readlock);
    //rm
    if (lockingStatus == -1)
    {
        perror("Error obtaining read lock on account record!");
        return false;
    }

    readBytes = read(accfd, &account, sizeof(struct Account));
   
    readlock.l_type = F_UNLCK;
    fcntl(accfd, F_SETLK, &readlock);

    if (customerAccount != NULL)
    {
        *customerAccount = account;
        return true;
    }

  
   memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    sprintf(msg.writeBuffer, "Account Details - \n\tAccount Number : %d\n\tAccount Type : %s\n\tAccount Status : %s", account.accountNumber, (account.isnormal ? "Regular" : "Joint"), (account.active) ? "Active" : "Deactived");
    if (account.active)
    {
        sprintf(tempBuffer, "\n\tAccount Balance:₹ %ld", account.balance);
        strcat(msg.writeBuffer, tempBuffer);
    }

    sprintf(tempBuffer, "\n\tPrimary Owner ID: %d", account.owners[0]);
    strcat(msg.writeBuffer, tempBuffer);
    if (account.owners[1] != -1)
    {
        sprintf(tempBuffer, "\n\tSecondary Owner ID: %d", account.owners[1]);
        strcat(msg.writeBuffer, tempBuffer);
    }

    strcat(msg.writeBuffer, "\n");

   
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
   

    return true;
}

bool get_customer_details(int connFD, int customerID)
{
    ssize_t readBytes, writeBytes;             
    char readBuffer[1000];//writeBuffer[10000]; 
     struct message msg;
    char tempBuffer[1000];

    struct Customer customer;
    int custfd;
    if (customerID == -1)//should it be Null 
    {
       memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"enter customer ID:\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));

        memset(readBuffer, 0,sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        

        customerID = atoi(readBuffer);
    }

    custfd = open("./records/customer.bank", O_RDONLY);
    if (custfd == -1 || customerID==0)
    {
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"invalid customer id\n");
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
       
        return false;
    }
    //check oofset 
    off_t end=lseek(custfd,0,SEEK_END);
    off_t offset = lseek(custfd, (customerID-1) * sizeof(struct Customer), SEEK_SET);
    //printf("%ld",offset);
    if (errno == EINVAL || end<=offset)
    {
        
      
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"invalid customer id\n" );
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        
    
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }   
    
        struct flock readlock;
        readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Customer); 
	readlock.l_pid = getpid(); 

    int lockingStatus = fcntl(custfd, F_SETLKW, &readlock);
   //rm
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Customer file!");
        return false;
    }
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(custfd, &customer, sizeof(struct Customer));
  

    readlock.l_type = F_UNLCK;
    fcntl(custfd, F_SETLK, &readlock);

    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    sprintf(msg.writeBuffer, "Customer Details - \n\tID : %d\n\tFirst Name : %s\n\tlast Name : %s\n\tGender : %c\n\tAge: %d\n\tAccount Number : %d\n\tLoginID : %s", customer.id, customer.firstname,customer.lastname, customer.gender, customer.age, customer.account, customer.login);

    strcat(msg.writeBuffer, "\n\nRedirecting to main menu....\n");
    
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
    
 
    return true;
}

bool get_transaction_details(int connFD, int accountNumber)
{

    ssize_t readBytes, writeBytes;                               
    char readBuffer[1000];
    //writeBuffer[10000], 
    struct message msg;
    
    char tempBuffer[1000]; 
    
    struct Account account;

    if (accountNumber == -1)
    {
        // Get the accountNumber
       memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"enter your account number\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        memset(readBuffer, 0,sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        account.accountNumber = atoi(readBuffer);
    }
    else
        account.accountNumber = accountNumber;

    if (get_account_details(connFD, &account))
    {
        int iter;

        struct Transaction transaction;
        struct tm transtime ;

       memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        int tranfd = open("./records/transactions.bank", O_RDONLY);
        
        if (tranfd == -1)
        {
            perror("Error while opening transaction file!");
          
        strcpy(msg.writeBuffer,"transaction not found\n" );
        msg.response=0;
        write(connFD, &msg, sizeof(msg));    
            
            return false;
        }
        
        
         off_t end=lseek(tranfd,0,SEEK_END);
        for (iter = 0; iter < MAX_TRANSACTIONS && account.transactions[iter] != -1; iter++)
        {
            //offset check
            int offset = lseek(tranfd, account.transactions[iter] * sizeof(struct Transaction), SEEK_SET);
            //rm
            if (offset == -1 || end<=offset)
            {
                perror("Error while seeking to required transaction record!");
                return false;
            }

          
            struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Transaction); 
	readlock.l_pid = getpid(); 
            
            
            int lockingStatus = fcntl(tranfd, F_SETLKW, &readlock);
            //rm
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on transaction record!");
                return false;
            }

            memset(readBuffer, 0,sizeof(readBuffer));
            readBytes = read(tranfd, &transaction, sizeof(struct Transaction));
          
            readlock.l_type = F_UNLCK;
            fcntl(tranfd, F_SETLK, &readlock);

            transtime  = *localtime(&(transaction.transtime ));

         memset(tempBuffer, 0,sizeof(tempBuffer));
            
            
            sprintf(tempBuffer, "Details of transaction %d - \n\t Date : %d:%d %d/%d/%d \n\t Operation : %s \n\t Balance - \n\t\t Before : %ld \n\t\t After : %ld \n\t\t transaction amount : %ld\n", (iter + 1), transtime.tm_hour, transtime.tm_min, transtime.tm_mday, (transtime .tm_mon + 1), (transtime.tm_year + 1900), (transaction.operation ? "Deposit" : "Withdraw"), transaction.oldBalance, transaction.newBalance, (transaction.newBalance - transaction.oldBalance));

            if (strlen(msg.writeBuffer) == 0)
                strcpy(msg.writeBuffer, tempBuffer);
            else
                strcat(msg.writeBuffer, tempBuffer);
        }

        close(tranfd);

        if (strlen(msg.writeBuffer) == 0)
        {
          
            strcpy(msg.writeBuffer, "transaction not found\n");
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
            return false;
        }
        else
        {    msg.response=0;
        write(connFD, &msg, sizeof(msg));
         
          
        }
    }
}


