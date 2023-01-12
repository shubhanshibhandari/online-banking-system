

#include "./common.h"
#include <sys/ipc.h>
#include <sys/sem.h>
int semid;
/*union semun{
 int val;
 struct semid_ds *buf;
 unsigned short int *array;

};*/
struct sembuf semOp1;


bool admin_operation_handler(int connFD);
bool add_account(int connFD);
int add_customer(int connFD, bool isPrimary, int newAccountNumber);
bool delete_account(int connFD);
bool modify_customer_info(int connFD);


bool admin_operation_handler(int connFD)
{

    if (login_handler(true, connFD, NULL))
    {
        ssize_t writeBytes, readBytes;            
        
        char readBuffer[1000];
        
        struct message msg;
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "admin login successful!!\n");
      
         msg.response=0;
        write(connFD, &msg, sizeof(msg));
        while (1)
        {
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer,"\n1. Get Customer Details\n2. Get Account Details\n3. Get Transaction details\n4. Add Account\n5. Delete Account\n6. Modify Customer Information\nPress any other key to logout\n");
           
            msg.response=1;
            write(connFD, &msg, sizeof(msg));
         
            
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            read(connFD, readBuffer, sizeof(readBuffer));
  

            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                get_customer_details(connFD, -1);
                break;
            case 2:
                get_account_details(connFD, NULL);
                break;
            case 3: 
                get_transaction_details(connFD, -1);
                break;
            case 4:
                add_account(connFD);
                break;
            case 5:
                delete_account(connFD);
                break;
            case 6:
                modify_customer_info(connFD);
                break;
            default:
               
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"admin logged out.Reopen application\n" );
                msg.response=0;
                write(connFD, &msg, sizeof(msg));
                return false;
            }
        }
    }
    else
    {    
        // ADMIN LOGIN FAILED
       
        return false;
    }
    return true;
}

bool add_account(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000];
    
    struct message msg;
    struct Account newAccount, prevAccount;
     int accfd;
     
    // add semaphore
    // add semaphore
      key_t key_admin = ftok(".",'a');    
      union semun arg1;
      int semid_admin;
        int semctlStatus;
        semid_admin = semget(key_admin, 1, 0); 
        if (semid_admin == -1)
        {
            semid_admin = semget(key_admin, 1, IPC_CREAT | 0744); 
            //rm
            if (semid_admin == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            arg1.val = 1; 
            semctlStatus = semctl(semid_admin, 0, SETVAL, arg1);
            
            //rm
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }  
   
    //struct sembuf semOp1; // Defines the operation on the semaphore
    semOp1.sem_num = 0;
    semOp1.sem_flg = SEM_UNDO;
    semOp1.sem_op = -1;
    semop(semid_admin, &semOp1, 1);
     
    accfd = open("./records/account.bank", O_RDONLY);
    if (accfd == -1 && errno == ENOENT)
    {
        
        newAccount.accountNumber = 54000;
    }
    else if (accfd == -1)
    {
        perror("Error while opening account file");
        semOp1.sem_op = 1;
        semop(semid_admin, &semOp1, 1);
   
        return false;
    }
    else
    {
        int offset = lseek(accfd, -sizeof(struct Account), SEEK_END);
        
        //rm
        if (offset == -1)
        {
            perror("Error seeking to last Account record!");
            semOp1.sem_op = 1;
        semop(semid_admin, &semOp1, 1);
            return false;
        }
        
        struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Account); 
	readlock.l_pid = getpid(); 
	
        
        int lockingStatus = fcntl(accfd, F_SETLKW, &readlock); 
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Account record!");
            semOp1.sem_op = 1;
            semop(semid_admin, &semOp1, 1);
   
            return false;
        }

        readBytes = read(accfd, &prevAccount, sizeof(struct Account));
        

        readlock.l_type = F_UNLCK;
        fcntl(accfd, F_SETLK, &readlock);

        close(accfd);

        newAccount.accountNumber = prevAccount.accountNumber + 1;
    }
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    strcpy(msg.writeBuffer," account type : Enter 1 for normal account and any other for joint account\n");
  
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
   
    
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
   

    newAccount.isnormal = atoi(readBuffer) == 1 ? true : false;

    newAccount.owners[0] = add_customer(connFD, true, newAccount.accountNumber);
    if(newAccount.owners[0]==-1)
    {
      semOp1.sem_op = 1;
     semop(semid_admin, &semOp1, 1);
   
    return false;
    }
    if (newAccount.isnormal)
        newAccount.owners[1] = -1;
    else{
        newAccount.owners[1] = add_customer(connFD, false, newAccount.accountNumber);
         if(newAccount.owners[1]==-1)
         {             semOp1.sem_op = 1;
                       semop(semid_admin, &semOp1, 1);
           return false;
         }
         }
    newAccount.active = true;
    newAccount.balance = 0;

    memset(newAccount.transactions, -1, MAX_TRANSACTIONS * sizeof(int));
    close(accfd);
    //use semaphore instead??
    
    accfd = open("./records/account.bank", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    
    // or use semaphore
    //can add write lock here on the record choose between mandatory locking or record 
    //int offset = lseek(accfd,0, SEEK_END);
        struct flock writelock;
        
        writelock.l_type = F_WRLCK; 
	writelock.l_whence = SEEK_SET; 
	writelock.l_start = 0; 
	writelock.l_len =0; 
	writelock.l_pid = getpid(); 
	fcntl(accfd, F_SETLKW, &writelock);   
    
    
    //rm
    if (accfd == -1)
    {
        perror("Error while creating / opening account file!");
            semOp1.sem_op = 1;
            semop(semid_admin, &semOp1, 1);        
        return false;
    }

    writeBytes = write(accfd, &newAccount, sizeof(struct Account));
   
    
    writelock.l_type = F_UNLCK; 
    fcntl(accfd, F_SETLK, &writelock); 
    
    
    close(accfd);
    semOp1.sem_op = 1;
   semop(semid_admin, &semOp1, 1);
  
    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
   
    sprintf(msg.writeBuffer, "customer's account's number is:%d\n", newAccount.accountNumber);
    
    strcat(msg.writeBuffer, "\n\t\t\tRedirecting to the main menu ...\n");

    msg.response=0;
    write(connFD, &msg, sizeof(msg));
    return true;
}

int add_customer(int connFD, bool isPrimary, int newAccountNumber)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000];
 
    struct message msg;
    struct Customer newCustomer, previousCustomer;
    int custfd;
    custfd = open("./records/customer.bank", O_RDONLY);
    if (custfd == -1 && errno == ENOENT)
    {
       
        newCustomer.id = 1;
    }
    else if (custfd == -1)
    {
        perror("Error while opening customer file");
        return -1;
    }
    else
    {
        int offset = lseek(custfd, -sizeof(struct Customer), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Customer record!");
            return -1;
        }
         struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Customer); 
	readlock.l_pid = getpid(); 
	
        
         fcntl(custfd, F_SETLKW, &readlock);
       
        readBytes = read(custfd, &previousCustomer, sizeof(struct Customer));
        
       

        readlock.l_type = F_UNLCK;
        fcntl(custfd, F_SETLK, &readlock);

        close(custfd);

        newCustomer.id = previousCustomer.id + 1;
    }

    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    if (isPrimary)
        sprintf(msg.writeBuffer, "%s%s", "Enter the details for the primary customer\n","enter customer's first name\n" );
    else
        sprintf(msg.writeBuffer, "%s%s", "Enter the details for the secodary customer\n","enter customer's first name\n");
  
    
    msg.response=1;
    write(connFD, &msg, sizeof(msg));

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    
    

    strcpy(newCustomer.firstname, readBuffer);
    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    
   
    strcpy(msg.writeBuffer, "enter customer's lastname\n");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        memset(readBuffer, 0,sizeof(readBuffer));
      readBytes = read(connFD, readBuffer, sizeof(readBuffer));
       strcpy(newCustomer.lastname, readBuffer);
    
    
    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"customer's gender:\nEnter M for male, F for female and O for others\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));

 
    
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
   
    if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O')
        newCustomer.gender = readBuffer[0];
    else
    {
      
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "invalid choice\nredirecting to the main menu!\n");
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        
       
        return -1;
    }


    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    strcpy(msg.writeBuffer,"enter customer's age\n");
   
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
  
    
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    

    int customerAge = atoi(readBuffer);
    
    if (customerAge <= 0 )
    {
        // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
 
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "invalid age\nredirecting to main menu!\n");
      
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        
       
        return -1;
    }
    newCustomer.age = customerAge;

    newCustomer.account = newAccountNumber;

    strcpy(newCustomer.login, newCustomer.firstname);
    strcat(newCustomer.login, "_");
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    sprintf(msg.writeBuffer, "%d", newCustomer.id);
    strcat(newCustomer.login, msg.writeBuffer);
    
    
  
    strcpy(newCustomer.password, "password");
    
    custfd = open("./records/customer.bank", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (custfd == -1)
    {
        perror("Error while creating / opening customer file!");
        return -1;
    }
    
    

    writeBytes = write(custfd, &newCustomer, sizeof(newCustomer));
   
    close(custfd);

  
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    
    sprintf(msg.writeBuffer, "%s%s%s%d\n%s%s\n%s", "The autogenerated login ID is : ", newCustomer.firstname,"_",newCustomer.id,"autogenerated password is :", "password","kindly change your id password at first login\n");
    
 
    msg.response=0;
    write(connFD, &msg, sizeof(msg));
    return newCustomer.id;
}

bool delete_account(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000];
    
    struct message msg;
    struct Account account;

  
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"enter account no to be deleted:\n");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));

    
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
   

    int accountNumber = atoi(readBuffer);

    int accfd = open("./records/account.bank", O_RDONLY);
    if (accfd == -1)
    {
        // Account record doesn't exist
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "Account record does not exists\n");
      
        
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        
       
     
        return false;
    }

    off_t end=lseek(accfd,0,SEEK_END);
    int offset = lseek(accfd, (accountNumber-54000) * sizeof(struct Account), SEEK_SET);
    
    
    
    if (errno == EINVAL || end<=offset)//offset check
    {
        // Customer record doesn't exist
       
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"cannot find account record\n");
        msg.response=0;
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
        perror("Error obtaining read lock on Account record!");
        return false;
    }

    readBytes = read(accfd, &account, sizeof(struct Account));

    readlock.l_type = F_UNLCK;
    fcntl(accfd, F_SETLK, &readlock);

    close(accfd);

   
    
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    if (account.balance == 0)
    {
        // No money, hence can close account
        account.active = false;
        accfd = open("./records/account.bank", O_WRONLY);
        //rm
        if (accfd == -1)
        {
            perror("Error opening Account file in write mode!");
            return false;
        }

        offset = lseek(accfd, (accountNumber-54000)* sizeof(struct Account), SEEK_SET);
        //rm
        if (offset == -1)
        {
            perror("Error seeking to the Account!");
            return false;
        }

        readlock.l_type = F_WRLCK;
        readlock.l_start = offset;

        int lockingStatus = fcntl(accfd, F_SETLKW, &readlock);
        //rm
        if (lockingStatus == -1)
        {
            perror("Error obtaining write lock on the Account file!");
            return false;
        }

        writeBytes = write(accfd, &account, sizeof(struct Account));
        

        readlock.l_type = F_UNLCK;
        fcntl(accfd, F_SETLK, &readlock);

        strcpy(msg.writeBuffer,"Account deleted successfully!\nRedirecting to main menu\n");
    }
    else{
        // Account has some money ask customer to withdraw it
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "cannot delete the account as it still has some money in it.Kindly first withdraw then delete\n");
        }
   
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
    return true;
}

bool modify_customer_info(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000];

    struct message msg;
    struct Customer customer;

    int customerID;

    off_t offset;
    int lockingStatus;


    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"enter customer id:\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
   
    
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    
  

    customerID = atoi(readBuffer);

    int custfd = open("./records/customer.bank", O_RDONLY);
    if (custfd == -1 || customerID==0)
    {
        // Customer File doesn't exist
       
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"no record exists\n");
       
       
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        return false;
    }
    off_t end=lseek(custfd,0,SEEK_END);
    offset = lseek(custfd, (customerID-1) * sizeof(struct Customer), SEEK_SET);
    //printf("%ld",offset);
    if (errno == EINVAL ||  end<=offset)
    {
        // Customer record doesn't exist
        
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "given customer id doesnt exists\n");
       
       
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
   
    lockingStatus = fcntl(custfd, F_SETLKW, &readlock);
    //rm
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on customer record!");
        return false;
    }
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(custfd, &customer, sizeof(struct Customer));
   
    // Unlock the record
    readlock.l_type = F_UNLCK;
    fcntl(custfd, F_SETLK, &readlock);

    close(custfd);
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
    
    
    strcpy(msg.writeBuffer,"Select information to modify:\n1.First name \n2. Last Name \n3.Age\n 4.Gender\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
  
    
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    
   

    int choice = atoi(readBuffer);
    if (choice == 0)
    { // A non-numeric string was passed to atoi
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        
        strcpy(msg.writeBuffer, "inavlid choice.\nRedirecting to main menu\n");
        
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        
        return false;
    }


    
    memset(readBuffer, 0,sizeof(readBuffer));
    switch (choice)
    {
    case 1:
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));

      
        strcpy(msg.writeBuffer,"enter customer new first name:\n");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
       
        strcpy(customer.firstname, readBuffer);
        break;
        
    case 2:
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));

       
        strcpy(msg.writeBuffer,"enter customer new last name:\n");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    
        
        strcpy(customer.lastname, readBuffer);
        break;    
    case 3:
    
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
      
        strcpy(msg.writeBuffer,"enter customer new age:\n");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
       
        
        int updatedAge = atoi(readBuffer);
        if (updatedAge <= 0)
        {
            
            
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer,"invalid age!\nredirecting to main menu\n");
            //writeBytes = write(connFD, msg.writeBuffer, strlen(msg.writeBuffer));
            msg.response=0;
            write(connFD, &msg, sizeof(msg));
            //readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        customer.age = updatedAge;
        break;
    case 4:
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
       

        strcpy(msg.writeBuffer,"enter new gender: M for male F for female and O for other\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));      
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        
             
         
         if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O')
        customer.gender = readBuffer[0];
    else
    {
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"invalid choice!\nredirecting to the main menu!\n");
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        
      
        return false;
    }

        //customer.gender = readBuffer[0];
        break;
    default:
        
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"invalid choice!\nredirecting to main menu\n");
      
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
        return false;
    }

    custfd = open("./records/customer.bank", O_WRONLY);
    if (custfd == -1)
    {
        perror("Error while opening customer file");
        return false;
    }
    offset = lseek(custfd, (customerID-1) * sizeof(struct Customer), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }

    readlock.l_type = F_WRLCK;
    readlock.l_start = offset;
    lockingStatus = fcntl(custfd, F_SETLKW, &readlock);
   //rm
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on customer record!");
        return false;
    }

    writeBytes = write(custfd, &customer, sizeof(struct Customer));
    

    readlock.l_type = F_UNLCK;
    fcntl(custfd, F_SETLK, &readlock);

    close(custfd);

    
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"the customer details modified successfully!\nredirecting to main menu\n");
        msg.response=0;
        write(connFD, &msg, sizeof(msg));
    
  

    return true;
}

