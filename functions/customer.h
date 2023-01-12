

#include <sys/ipc.h>
#include <sys/sem.h>


struct Customer logincust;
int semid;
int semid_trans;

/*union semun{
 int val;
 struct semid_ds *buf;
 unsigned short int *array;

};
*/
struct sembuf semOp;
struct sembuf semOp2;

bool customer_operation_handler(int connFD);
bool deposit(int connFD);
bool withdraw(int connFD);
bool get_balance(int connFD);
bool change_password(int connFD);




bool customer_operation_handler(int connFD)
{
    if (login_handler(false, connFD, &logincust))
    {
        ssize_t writeBytes, readBytes;            
        char readBuffer[1000];//writeBuffer[1000]; 
        struct message msg;
      
        key_t key = ftok("./records/customer.bank", logincust.account); 

        union semun arg;

        int semctlStatus;
        semid = semget(key, 1, 0); 
        if (semid == -1)
        {
            semid = semget(key, 1, IPC_CREAT | 0744); 
            //rm
            if (semid == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            arg.val = 1; 
            semctlStatus = semctl(semid, 0, SETVAL, arg);
            
            //rm
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }
        
        key_t key_trans = ftok(".",'t');    
        union semun arg2;
        //int semid_trans;
        //int semctlStatus;
        semid_trans = semget(key_trans, 1, 0); 
        if (semid_trans == -1)
        {
            semid_trans = semget(key_trans, 1, IPC_CREAT | 0744); 
            //rm
            if (semid_trans == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            arg2.val = 1; 
            semctlStatus = semctl(semid_trans, 0, SETVAL, arg2);
            
            //rm
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }  
           

       
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"customer login successful!");
        while (1)
        {
            strcat(msg.writeBuffer, "\n");
            strcat(msg.writeBuffer,"1. Get Customer Details\n2. Deposit\n3. Withdraw\n4. Get Balance\n5. Get Transaction information\n6. Change Password\nPress any other key to logout\n");
           
            msg.response=1;
            write(connFD, &msg, sizeof(msg));
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            memset(readBuffer, 0,sizeof(readBuffer));
            
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
           
            int choice = atoi(readBuffer);
          
            switch (choice)
            {
            case 1:
                get_customer_details(connFD, logincust.id);
                break;
            case 2:
                deposit(connFD);
                break;
            case 3:
                withdraw(connFD);
                break;
            case 4:
                get_balance(connFD);
                break;
            case 5:
                get_transaction_details(connFD, logincust.account);
                break;
            case 6:
                change_password(connFD);
                break;
            default:
                //writeBytes = write(connFD,"logout successful\n",19);
                strcpy(msg.writeBuffer,"logout successful\n.Reopen the application" );
                msg.response=0;
                write(connFD, &msg, sizeof(msg));
                semOp3.sem_op = 1;
                semop(semid_customer, &semOp3, 1); 
                return false;
            }
        }
    }
    else
    {
        // CUSTOMER LOGIN FAILED
        return false;
    }
    return true;
}

bool deposit(int connFD)
{
    char readBuffer[1000];//msg.writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    struct message msg;
    struct Account account;
    account.accountNumber = logincust.account;

    long int dep = 0;
   semOp.sem_num = 0;
    semOp.sem_flg = SEM_UNDO;
    semOp.sem_op = -1;
    semop(semid, &semOp, 1); 
  
  
    
    
    if (get_account_details(connFD, &account))
    {
        
        if (account.active)
        {

           
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer, "enter amount for deposit\n");
            msg.response=1;
            write(connFD, &msg, sizeof(msg));
            
            
            
            memset(readBuffer, 0,sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            

            dep = atol(readBuffer);
            if (dep > 0)
            { 
            
             semOp2.sem_num = 0;
    semOp2.sem_flg = SEM_UNDO;
    semOp2.sem_op = -1;
    semop(semid_trans, &semOp2, 1);

               int newTransactionID; 
                
    struct Transaction newTransaction;
    newTransaction.accountNumber = account.accountNumber;
    newTransaction.oldBalance = account.balance;
    newTransaction.newBalance = account.balance + dep;
    newTransaction.operation = 1;
    newTransaction.transtime  = time(NULL);

    ssize_t readBytes, writeBytes;

    int tranfd = open("./records/transactions.bank", O_CREAT | O_APPEND | O_RDWR, S_IRWXU);

  // add semaphore 
  
  
    off_t offset = lseek(tranfd, -sizeof(struct Transaction), SEEK_END);
       struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Transaction); 
	readlock.l_pid = getpid(); 
	
        
        fcntl(tranfd, F_SETLKW, &readlock);   
    
    if (offset >= 0)
    {   
        
        struct Transaction prevTransaction;
        readBytes = read(tranfd, &prevTransaction, sizeof(struct Transaction));

        newTransaction.transactionID = prevTransaction.transactionID + 1;
    }
    else
       
        newTransaction.transactionID = 0;
        
        readlock.l_type=F_UNLCK;
        fcntl(tranfd, F_SETLK, &readlock);
        
        
        //can add write lock
    
    writeBytes = write(tranfd, &newTransaction, sizeof(struct Transaction));

     newTransactionID=newTransaction.transactionID;

                
                
                
                int iter = 0;
    while (account.transactions[iter] != -1)
        iter++;

    if (iter >= MAX_TRANSACTIONS)
    {
              for (iter = 1; iter < MAX_TRANSACTIONS; iter++)
            account.transactions[iter - 1] = account.transactions[iter];
        account.transactions[iter - 1] = newTransactionID;
    }
    else
    {
        
        account.transactions[iter] = newTransactionID;
    }
                
                
                account.balance += dep;

                int accfd = open("./records/account.bank", O_WRONLY);
                offset = lseek(accfd, (account.accountNumber-54000) * sizeof(struct Account), SEEK_SET);

                
                struct flock writelock; 
	        writelock.l_type = F_WRLCK; 
	        writelock.l_whence = SEEK_SET; 
	        writelock.l_start = offset; 
	        writelock.l_len = sizeof(struct Account); 
	        writelock.l_pid = getpid(); 
	        
                
                int lockingStatus = fcntl(accfd, F_SETLKW, &writelock);
                
                //rm
                if (lockingStatus == -1)
                {
                    perror("Error obtaining write lock on account file!");
                    
                    semOp.sem_op = 1;
                    semop(semid, &semOp, 1);
                    semOp2.sem_op = 1;
                semop(semid_trans, &semOp2, 1);
                    return false;
                }

                writeBytes = write(accfd, &account, sizeof(struct Account));
                
                                
                writelock.l_type = F_UNLCK;
                fcntl(accfd, F_SETLK, &writelock);
                semOp2.sem_op = 1;
                semop(semid_trans, &semOp2, 1);
                
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer, "transaction successfull\n");
                msg.response=0;
                write(connFD, &msg, sizeof(msg));                

             

                get_balance(connFD);

               
                semOp.sem_op = 1;
                semop(semid, &semOp, 1);
                
                
                return true;
            }
            else{
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"invalid deposit amount!\n");
                msg.response=0;
                write(connFD, &msg, sizeof(msg));
             
        }
        }
        else{
          
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"account is deactive\n");
                msg.response=0;
                write(connFD, &msg, sizeof(msg));
      }
        semOp.sem_op = 1;
        semop(semid, &semOp, 1);       
    }
    else
    {
       
        
        semOp.sem_op = 1;
        semop(semid, &semOp, 1);
        return false;
    }
}

bool withdraw(int connFD)
{
    char readBuffer[1000];
    // writeBuffer[1000];
    ssize_t readBytes, writeBytes;
     struct message msg;
    struct Account account;
    account.accountNumber = logincust.account;

    long int withd = 0;

  
    struct sembuf semOp; // Defines the operation on the semaphore
    semOp.sem_num = 0;
    semOp.sem_flg = SEM_UNDO;
    semOp.sem_op = -1;
    semop(semid, &semOp, 1);
    if (get_account_details(connFD, &account))
    {
        if (account.active)
        {  
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer,"enter amount to withdraw\n" );
            msg.response=1;
            write(connFD, &msg, sizeof(msg));
          
           

            
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            memset(readBuffer, 0,sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            

            withd = atol(readBuffer);

            if (withd != 0 && account.balance - withd >= 0)
            {
              semOp2.sem_num = 0;
    semOp2.sem_flg = SEM_UNDO;
    semOp2.sem_op = -1;
    semop(semid_trans, &semOp2, 1);
               int newTransactionID;
                struct Transaction newTransaction;
    newTransaction.accountNumber = account.accountNumber;
    newTransaction.oldBalance = account.balance;
    newTransaction.newBalance = account.balance-withd;
    newTransaction.operation = 0;
    newTransaction.transtime  = time(NULL);

    ssize_t readBytes, writeBytes;

    int tranfd = open("./records/transactions.bank", O_CREAT | O_APPEND | O_RDWR, S_IRWXU);

    off_t offset = lseek(tranfd, -sizeof(struct Transaction), SEEK_END);
       struct flock readlock; 
	
	
	readlock.l_type = F_RDLCK; 
	readlock.l_whence = SEEK_SET; 
	readlock.l_start = offset; 
	readlock.l_len = sizeof(struct Transaction); 
	readlock.l_pid = getpid(); 
	
        
        fcntl(tranfd, F_SETLKW, &readlock);   
    
    if (offset >= 0)
    {   
        struct Transaction prevTransaction;
        readBytes = read(tranfd, &prevTransaction, sizeof(struct Transaction));

        newTransaction.transactionID = prevTransaction.transactionID + 1;
    }
    else
        // No transaction records exist
        newTransaction.transactionID = 0;
        
        readlock.l_type=F_UNLCK;
        fcntl(tranfd, F_SETLK, &readlock);
        
        
        //can add write lock
    writeBytes = write(tranfd, &newTransaction, sizeof(struct Transaction));

     newTransactionID=newTransaction.transactionID;
                
                
                
               int iter = 0;
    while (account.transactions[iter] != -1)
        iter++;

    if (iter >= MAX_TRANSACTIONS)
    {
        // No space
        for (iter = 1; iter < MAX_TRANSACTIONS; iter++)
            // Shift elements one step back discarding the oldest transaction
            account.transactions[iter - 1] = account.transactions[iter];
        account.transactions[iter - 1] = newTransactionID;
    }
    else
    {
        // Space available
        account.transactions[iter] = newTransactionID;
    }
               
               
                account.balance -= withd;

                int accfd = open("./records/account.bank", O_WRONLY);
                offset = lseek(accfd, (account.accountNumber-54000) * sizeof(struct Account), SEEK_SET);
                 struct flock writelock;
              
                
                
                writelock.l_type = F_WRLCK; 
	        writelock.l_whence = SEEK_SET; 
	        writelock.l_start = offset; 
	        writelock.l_len = sizeof(struct Account); 
	        writelock.l_pid = getpid(); 
	
                
                int lockingStatus = fcntl(accfd, F_SETLKW, &writelock);
                //rm
                if (lockingStatus == -1)
                {
                    perror("Error obtaining write lock on account record!");
                   
                    semOp.sem_op = 1;
                    semop(semid, &semOp, 1);
                    semOp2.sem_op = 1;
                semop(semid_trans, &semOp2, 1);
                    return false;
                }

                writeBytes = write(accfd, &account, sizeof(struct Account));
               

                writelock.l_type = F_UNLCK;
                fcntl(accfd, F_SETLK, &writelock);
                semOp2.sem_op = 1;
                semop(semid_trans, &semOp2, 1);
                
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"transaction successful\n" );
                msg.response=0;
                write(connFD, &msg, sizeof(msg));
              

                get_balance(connFD);
                

                semOp.sem_op = 1;
                semop(semid, &semOp, 1);
                return true;
            }
            else{
               
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"invalid withdrawal amount\n" );
                msg.response=0;
                write(connFD, &msg, sizeof(msg));
        
        }
        }
        else{
           
                memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
                strcpy(msg.writeBuffer,"account is deactivated\n" );
                msg.response=0;
                write(connFD, &msg, sizeof(msg));   
        }
           semOp.sem_op = 1;
           semop(semid, &semOp, 1); 
    }
    else
    {
           semOp.sem_op = 1;
           semop(semid, &semOp, 1);
        return false;
    }
}

bool get_balance(int connFD)
{
    //char buffer[1000];
    struct message msg;
    struct Account account;
    account.accountNumber = logincust.account;
    if (get_account_details(connFD, &account))
    {
      
      
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        if (account.active)
        {
            sprintf(msg.writeBuffer, "Your current balance is ₹ %ld", account.balance);
           
        msg.response=0;
        write(connFD, &msg, sizeof(msg)); 
        }
        else{
           
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"account is deactivated\n" );
        msg.response=0;
        write(connFD, &msg, sizeof(msg));    
    
    }
    }
    else
    {
      
        return false;
    }
}

bool change_password(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000];//writeBuffer[1000]; 
    struct message msg;
    char newPassword[1000];

     
     struct sembuf semOp; 
    semOp.sem_num = 0;
    semOp.sem_flg = SEM_UNDO;
    
    
    semOp.sem_op = -1;
    semop(semid, &semOp, 1);
    
    //writeBytes = write(connFD,"enter current password\n ",25);
    memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer, "enter current password\n ");
        msg.response=1;
        write(connFD, &msg, sizeof(msg));

    
    
    memset(readBuffer, 0,sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    

    if (strcmp(readBuffer, logincust.password) == 0)
    {
       
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
        strcpy(msg.writeBuffer,"enter new password\n" );
        msg.response=1;
        write(connFD, &msg, sizeof(msg));
        
        memset(readBuffer, 0,sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
       
        strcpy(newPassword,readBuffer);

       
        
            strcpy(logincust.password, newPassword);

            int custfd = open("./records/customer.bank", O_WRONLY);
            //rm
            if (custfd == -1)
            {
                perror("Error opening customer file!");
                
              semOp.sem_op = 1;
              semop(semid, &semOp, 1);                
                
                return false;
            }
            //offset check
            off_t end=lseek(custfd,0,SEEK_END);
            off_t offset = lseek(custfd, (logincust.id-1) * sizeof(struct Customer), SEEK_SET);
            if (offset == -1 || end<=offset)
            {
                perror("Error seeking to the customer record!");
                
                 semOp.sem_op = 1;
              semop(semid, &semOp, 1);  
                return false;
            }

          
            struct flock writelock; 
	
	

	writelock.l_type = F_WRLCK; 
	writelock.l_whence = SEEK_SET; 
	writelock.l_start = offset; 
	writelock.l_len = sizeof(struct Customer); 
	writelock.l_pid = getpid(); 
	
            
            int lockingStatus = fcntl(custfd, F_SETLKW, &writelock);
            
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on customer record!");
                
                
                 semOp.sem_op = 1;
              semop(semid, &semOp, 1);  
                return false;
            }

            writeBytes = write(custfd, &logincust, sizeof(struct Customer));
            

            writelock.l_type = F_UNLCK;
            fcntl(custfd, F_SETLK, &writelock);

            close(custfd);

           
            memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer,"password changed successfully\n" );
            msg.response=0;
            write(connFD, &msg, sizeof(msg));
         
           
              semOp.sem_op = 1;
              semop(semid, &semOp, 1);
            return true;
        
        
    }
    else
    {
        memset(msg.writeBuffer, 0,sizeof(msg.writeBuffer));
            strcpy(msg.writeBuffer,"incorrect password\n" );
            msg.response=0;
            write(connFD, &msg, sizeof(msg));
       
    }

  
    semOp.sem_op = 1;
    semop(semid, &semOp, 1);
    return false;
}

