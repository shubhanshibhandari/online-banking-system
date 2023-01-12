

struct Customer
{
    int id; // 0, 1, 2 ....
    char firstname[25];
    char lastname[25];
    char gender; // M -> Male, F -> Female, O -> Other
    int age;
    
    // Login Credentials
    char login[30]; // Format : name_id (name will the first word in the structure member `name`)
    char password[30];
    // Bank data
    int account; // Account number of the account the customer owns
};


