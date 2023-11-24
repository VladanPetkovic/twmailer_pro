#include "../include/twmailer-server.h"

const int BUFFER_SIZE = 1024;

MailServer::MailServer(int port)
{
    // allocate memory for buffer
    this->buffer = new char[BUFFER_SIZE];

    /****************************************************************************************/
    // init semaphore for synchronization of output of multiple processes to the cmd
    sem_unlink("output_semaphore"); // deleting named semaphore, if some exists
    this->semaphore = sem_open("output_semaphore", O_CREAT | O_EXCL, 0644, 1);
            // control_semaphore = name of semaphore
            // O_CREAT = the semaphore should be created, if it didn't exist
            // O_EXCL = combination with O_CREAT
            // 0644 = permission mode of semaphore
            // 1 = semaphore is unlocked
    if(this->semaphore == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    /****************************************************************************************/

    this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_socket == -1)
    {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    // setting socket options
    setSocketOptions();

    memset( & server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(this->server_socket, (struct sockaddr * ) & server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind error");
        close(this->server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(this->server_socket, 5) == -1)
    {
        perror("Listen error");
        close(this->server_socket);
        exit(EXIT_FAILURE);
    }

    logMessage("Server started on port " + std::to_string(port));
}
MailServer::~MailServer()
{
    // free allocated memory
    delete[] this->buffer;

    if (this->server_socket != -1)
    {
        close(this->server_socket);
    }

    // close and remove "output_semaphore"
    sem_close(this->semaphore);
    sem_unlink("output_semaphore");
}
void MailServer::setSocketOptions()
{
   if(setsockopt(this->server_socket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &(this->reuseValue),
                  sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reuseAddr");
      exit(EXIT_FAILURE);
   }

   if(setsockopt(this->server_socket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &(this->reuseValue),
                  sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reusePort");
      exit(EXIT_FAILURE);
   }
}
void MailServer::logMessage(const std::string & msg = "")
{
    std::time_t currentTime = std::time(nullptr);
    std::string currentTimeString = std::ctime(&currentTime);
    currentTimeString.erase(currentTimeString.size() - 1);

    // lock semaphore: synchronizing output to cmd from processes
    sem_wait(this->semaphore);

    std::cout << "[" << currentTimeString << "] " << msg << std::endl;

    // unlock semaphore
    sem_post(this->semaphore);
}
void MailServer::handleClient(int client_socket) 
{
    // sem_t *semaphore = sem_open("/twmailerServerSemaphore", 0);
    // if (semaphore == SEM_FAILED)
    // {
    //     perror("Error opening semaphore in child process");
    //     close(client_socket);
    //     exit(EXIT_FAILURE);
    // }

    // // Acquire semaphore before accessing shared resources
    // sem_wait(semaphore);
    
    int size = 0;
    do 
    {
        // handle receive errors
        size = recv(client_socket, this->buffer, BUFFER_SIZE - 1, 0);
        if (size == -1)
        {
            perror("Receive error.");
            break;
        }

        if (size == 0)
        {
            // client closed socket remotely
            break;
        }

        this->buffer[size] = '\0';  // Null-terminate buffer

        // receiving LOGIN
        if(strncmp(this->buffer, "LOGIN", 5) == 0)
        {
            handleLogin(client_socket);
        }
        // receiving SEND
        else if(strncmp(this->buffer, "SEND", 4) == 0)
        {
            handleSend(client_socket);
        }
        // receiving LIST
        else if(strncmp(this->buffer, "LIST", 4) == 0)
        {
            handleList(client_socket);
        }
        // receiving READ
        else if(strncmp(this->buffer, "READ", 4) == 0)
        {
            handleRead(client_socket);
        }
        // receiving DEL
        else if(strncmp(this->buffer, "DEL", 3) == 0)
        {
            handleDel(client_socket);
        } 

   } while (strcmp(this->buffer, "QUIT") != 0);

//    // Release semaphore after done
//     sem_post(semaphore);

//     // Cleanup
//     sem_close(semaphore);
//     close(client_socket);

}
void MailServer::handleLogin(int client_socket)
{
    std::string new_buffer;     // we do not use our this->buffer, because we are sending only 5 bytes
    std::string username;
    std::string password;

    if(this->buffer == nullptr) // buffer is empty
    {
        new_buffer = "ERR\n";
    } 
    else                        // buffer is not empty
    {
        username = getSenderOrReceiver(getStartPosOfString(SENDER));    // getting the string after first \n
        password = getSenderOrReceiver(getStartPosOfString(RECEIVER));  // getting the string after second \n     
        
        std::string clientIP = getClientIP(client_socket);

        if (isUserBlacklisted(username, clientIP))
        {
            logMessage("User " + username + " temporarily blacklisted.");
            sendErrorResponse(client_socket, "ERR\n");
            return;
        }

        if (authenticateWithLDAP(username, password))
        {
            logMessage("User authenticated with LDAP.");
            new_buffer = "OK\n";
            resetLoginAttempt(username);
        }
        else
        {
            logMessage("LDAP authentication failed.");
            new_buffer = "ERR\n";
            updateLoginAttempt(username, clientIP);
        }
    }
    
    memset(this->buffer, '\0', BUFFER_SIZE);
    if (send(client_socket, new_buffer.c_str(), new_buffer.size(), 0) == -1)
    {
        perror("Send error");
    }
}
bool MailServer::isUserBlacklisted(const std::string& username, const std::string& ip)
{
    if (loginAttempts.count(username) > 0) {
        auto& attempt = loginAttempts[username];
        if (difftime(std::time(nullptr), attempt.lastAttemptTime) < 60 && attempt.attempts >= 3 && attempt.ip == ip) {
            return true;
        }
    }
    return false;
}
void MailServer::resetLoginAttempt(const std::string& username)
{
    loginAttempts.erase(username);
}
void MailServer::sendErrorResponse(int client_socket, const std::string& message)
{
    if (send(client_socket, message.c_str(), message.size(), 0) == -1)
    {
        perror("Error: Send error");
    }
}
void MailServer::updateLoginAttempt(const std::string& username, const std::string& ip)
{
    auto& attempt = loginAttempts[username];
    attempt.ip = ip;

    if (difftime(std::time(nullptr), attempt.lastAttemptTime) > 60)
    {
        attempt.attempts = 1;
    } 
    else 
    {
        attempt.attempts++;
    }
    attempt.lastAttemptTime = std::time(nullptr);
}
std::string MailServer::getClientIP(int client_socket)
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(client_socket, (struct sockaddr *)&addr, &addr_size);
    char clientip[20];
    if (res != -1)
    {
        strcpy(clientip, inet_ntoa(addr.sin_addr));
        return std::string(clientip);
    }
    return "";
}
bool MailServer::authenticateWithLDAP(const std::string& username, const std::string& password)
{
    LDAP *ldap = nullptr;
    LDAPMessage *searchResult = nullptr;
    int connectResult;

    try
    {
        // Initialize LDAP connection
        connectResult = ldap_initialize(&ldap, "ldap://ldap.technikum.wien.at:389");
        if (connectResult != LDAP_SUCCESS)
        {
            logMessage("LDAP connection failed.");
            throw std::runtime_error("LDAP connection initialization failed");
        }

        // Set LDAP version
        int ldapVersion = LDAP_VERSION3;
        ldap_set_option(ldap, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);

        // Bind anonymously to perform search
        connectResult = ldap_sasl_bind_s(ldap, NULL, LDAP_SASL_SIMPLE, NULL, NULL, NULL, NULL);
        if (connectResult != LDAP_SUCCESS)
        {
            throw std::runtime_error("LDAP anonymous bind failed");
        }

        // Search for the user
        std::string searchFilter = "(uid=" + username + ")";
        connectResult = ldap_search_ext_s(ldap, "dc=technikum-wien,dc=at", LDAP_SCOPE_SUBTREE,
                                          searchFilter.c_str(), NULL, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &searchResult);

        if (connectResult != LDAP_SUCCESS)
        {
            throw std::runtime_error("LDAP search failed");
        }

        // Check if user exists and get DN
        LDAPMessage *entry = ldap_first_entry(ldap, searchResult);
        if (entry == NULL)
        {
            throw std::runtime_error("User not found in LDAP");
        }

        char *userDN = ldap_get_dn(ldap, entry);
        if (userDN == NULL)
        {
            throw std::runtime_error("Failed to get user DN");
        }

        // Attempt to bind as the user
        struct berval cred;
        cred.bv_val = const_cast<char*>(password.c_str());
        cred.bv_len = password.length();

        // Attempt to bind as the user
        connectResult = ldap_sasl_bind_s(ldap, userDN, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
        ldap_memfree(userDN);

        if (connectResult != LDAP_SUCCESS)
        {
            throw std::runtime_error("LDAP user bind failed");
        }

        // Successful authentication
        return true;
    }
    catch (const std::exception& e)
    {
        logMessage(std::string("LDAP authentication error: ") + e.what());
    }
    
    // Free LDAP resources in all scenarios
    if (searchResult) ldap_msgfree(searchResult);
    if (ldap) ldap_unbind_ext_s(ldap, NULL, NULL);

    return false;
}
void MailServer::handleSend(int client_socket)
{
    std::string new_buffer;     // we do not use our this->buffer, because we are sending only 5 bytes
    std::string sender;
    std::string receiver;

    if(this->buffer == nullptr) // buffer is empty
    {
        new_buffer = "ERR\n";
    } 
    else                        // buffer is not empty
    {
        sender = getSenderOrReceiver(getStartPosOfString(SENDER));
        receiver = getSenderOrReceiver(getStartPosOfString(RECEIVER));
        
        if(storeMessage(sender, receiver))
        {
            logMessage("Message saved.");
            new_buffer = "OK\n";
        }
        else
        {
            logMessage("Error while saving message.");
            new_buffer = "ERR\n";
        }
    }
    
    memset(this->buffer, '\0', BUFFER_SIZE);
    if (send(client_socket, new_buffer.c_str(), new_buffer.size(), 0) == -1)
    {
        perror("Send error");
    }
}
void MailServer::handleList(int client_socket)
{
    std::string new_buffer;
    std::string subject_buffer = "";
    std::string username = getSenderOrReceiver(getStartPosOfString(SENDER));
    std::string directoryName = this->message_store + "/" + username;

    DIR* dir = opendir(directoryName.c_str());
    struct dirent * entry; // struct for fileinput

    // buffer is empty
    if(this->buffer == nullptr)
    {
        new_buffer = "Count of messages: 0\n";
        send(client_socket, new_buffer.c_str(), new_buffer.size(), 0);
        return;
    }

    // folder of user does not exist
    if (dir == nullptr) 
    {
        new_buffer = "Count of messages: 0\n";
        send(client_socket, new_buffer.c_str(), new_buffer.size(), 0);
        return;
    }  

    // checking all entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // checking only regular files -- REG == regular file
        if(entry->d_type == DT_REG)
        {
            std::string line;
            std::string subject;
            // Open the file for reading
            std::ifstream file(directoryName + "/" + entry->d_name);
            int lineCountdown = 3;      // moving three lines down to SUBJECT

            while(file && std::getline(file, line))    // check with file if file opened successfully
            {
                lineCountdown--;
                if(lineCountdown == 0)
                {
                    subject = "Message-Number ";
                    subject += entry->d_name;
                    subject += ": " + line;
                    subject_buffer += subject + "\n";
                }
            }
        }
    }

    new_buffer = "Count of messages: " + std::to_string(getFileCount(username)) + "\n";
    new_buffer += subject_buffer;
    logMessage("Returned Message-List");

    send(client_socket, new_buffer.c_str(), new_buffer.size(), 0);
}
void MailServer::handleRead(int client_socket)
{
    if (this->buffer == nullptr)
    {
        std::string error_message = "ERR\n";
        send(client_socket, error_message.c_str(), error_message.size(), 0);
        return;
    }

    std::string username = getSenderOrReceiver(getStartPosOfString(SENDER));
    int messageNumber = getMessageNumber(getStartPosOfString(RECEIVER));    // RECEIVER means 2 times \n, which is true for msg-number
    std::string directoryName = this->message_store + "/" + username;
    std::string filePath = directoryName + "/" + std::to_string(messageNumber);
    std::ifstream file(filePath);
    std::string server_response;

    if (file.is_open())
    {
        std::string line;
        server_response = "OK\n";
        while(std::getline(file, line))
        {
            server_response += line + "\n";
        }
        send(client_socket, server_response.c_str(), server_response.size(), 0);
    }
    else
    {
        std::string error_message = "ERR\n";
        send(client_socket, error_message.c_str(), error_message.size(), 0);
    }
}
void MailServer::handleDel(int client_socket)
{
    if(buffer == nullptr)
    {
        std::string new_buffer = "ERR\n";
        send(client_socket, new_buffer.c_str(), new_buffer.size(), 0);
        return;
    }

    std::string username = getSenderOrReceiver(getStartPosOfString(SENDER));
    int messageNumber = getMessageNumber(getStartPosOfString(RECEIVER));    // RECEIVER means 2 times \n, which is true for msg-number
    std::string directoryName = this->message_store + "/" + username;
    std::string filePath = directoryName + "/" + std::to_string(messageNumber);
    std::string new_buffer;

    if(std::remove(filePath.c_str()) == 0)
    {
        logMessage("Message removed.");
        new_buffer = "OK\n";
    }
    else
    {
        logMessage();
        perror("Error deleting file");
        new_buffer = "ERR\n";
    }
    send(client_socket, new_buffer.c_str(), new_buffer.size(), 0);
}
int MailServer::getMessageNumber(int startOfMessageNumber)
{
    std::string numberString;

    for(int i = startOfMessageNumber; this->buffer[i] != '\0'; i++)
    {
        if(this->buffer[i] == '\n')
        {
            break;
        }
        numberString += this->buffer[i];
    }
    
    return std::stoi(numberString);
}
std::string MailServer::getSenderOrReceiver(int startPosition)
{
    std::string returnString;

    for(int i = startPosition; this->buffer[i] != '\0'; i++)
    {
        if(this->buffer[i] == '\n')
        {
            break;
        }
        returnString += this->buffer[i];
    }

    return returnString;
}
int MailServer::getServerSocket()
{
    return this->server_socket;
}
int MailServer::getStartPosOfString(LineBreakCount count)
{
    int stringStart;
    int newLineCharCount = 0;

    // reach char of receiver
    for(stringStart = 0; this->buffer[stringStart] != '\0'; stringStart++)
    {
        if(this->buffer[stringStart] == '\n')
        {
            newLineCharCount++;
            if(newLineCharCount == count)
            {
                stringStart++;
                break;
            }
        }
    }

    return stringStart;
}
int MailServer::getEndPosOfMessage(int startOfMessage)
{
    int messageEnd;

    for(messageEnd = startOfMessage; this->buffer[messageEnd] != '\0'; messageEnd++)
    {
        if( this->buffer[messageEnd] == '\n' && 
            this->buffer[messageEnd + 1] == '.' && 
            this->buffer[messageEnd + 2] == '\n')
        {
            break;
        }
    }

    return messageEnd - 1;
}
bool MailServer::storeMessage(std::string sender, std::string receiver)
{
    DIR* dir = opendir(this->message_store.c_str());
    struct dirent * ent; // struct for fileinput
    std::string directoryName = this->message_store + "/" + sender;
    bool folderFound = false;

    // checking if directory opening failed
    if(dir == nullptr)
    {
        std::cerr << "Cannot access " << this->message_store << ": No such directory" << std::endl;
        return false;
    }

    // checking all entries in the directory
    while ((ent = readdir(dir)) != NULL)
    {
        // if sender found as subdirectory --> create and write File
        if(ent->d_type == DT_DIR && ent->d_name == receiver)
        {
            folderFound = true;
            return createAndWriteFile(sender, receiver);
        }
    }

    // if folder was not found --> make new folder for user and call storeMessage() again
    if(!folderFound)
    {
        if (mkdir(directoryName.c_str(), 0777) == 0)
        {
            logMessage("Directory created successfully.");
            //   std::cout << "Directory created successfully." << std::endl;
            return createAndWriteFile(sender, receiver);
        }
        else
        {
            logMessage("Failed to create directory.");
            //   std::cout << "Failed to create directory." << std::endl;
            return false;
        }
    }

    // closing directory
    closedir(dir);

    return true;
}
int MailServer::getFileCount(std::string username)
{
    std::string directoryName = this->message_store + "/" + username;
    DIR* dir = opendir(directoryName.c_str());
    struct dirent * entry; // struct for fileinput
    int fileCount = 0;

    if (dir == nullptr) 
    {
        logMessage("Failed to open the directory.");
        //   std::cout << "Failed to open the directory." << std::endl;
        return 0;   // directory does not exist --> return 0
    } 
    else 
    {
        while ((entry = readdir(dir))) 
        {
            if (entry->d_type == DT_REG) // REG == regular file 
            {
                fileCount++;
            }
        }
        closedir(dir);
    }

    return fileCount;
}
int MailServer::getMaxMessageNumber(std::string username)
{
    std::string directoryName = this->message_store + "/" + username;
    DIR* dir = opendir(directoryName.c_str());
    struct dirent * entry; // struct for fileinput
    int maxNumber = 0;

    if (dir == nullptr) 
    {
        // directory does not exist --> return 0
        return 0;
    } 
    else 
    {
        while ((entry = readdir(dir))) 
        {
            if (entry->d_type == DT_REG && atoi(entry->d_name) > maxNumber) // REG == regular file 
            {
                maxNumber = atoi(entry->d_name);
            }
        }
        closedir(dir);
    }

    return maxNumber;
}
bool MailServer::createAndWriteFile(std::string sender, std::string receiver)
{
    int startPosOfMessage = getStartPosOfString(MESSAGE);
    int startPosOfSubject = getStartPosOfString(SUBJECT);
    std::string directoryName = this->message_store + "/" + receiver;

    // get max messageNumber and add 1 to it
    // we added + 1, because getFileCount() can be 0 --> we want to start with 1 (as message number)
    int messageNumber = getMaxMessageNumber(receiver) + 1;

    // Create a new file within the folder
    std::ofstream file(directoryName + std::string("/") + std::to_string(messageNumber));

    // opening and writing file
    if(file.is_open())
    {
        file << sender << "\n" << receiver << "\n";
        // write Subject
        for(    int i = startPosOfSubject; 
                this->buffer[i] != '\0' && i < getStartPosOfString(MESSAGE) - 1; i++)
        {
            file << buffer[i];
        }
        file << "\n";

        // write Message
        for(    int i = startPosOfMessage; 
                this->buffer[i] != '\0' && i < getEndPosOfMessage(startPosOfMessage); i++)
        {
            file << buffer[i];
        }
        file.close();
        // File created successfully
        return true;
    }
    else
    {
        // Failed to create the file
        return false;
    }
}
void MailServer::checkDirectory(const std::string & message_store)
{
    // init message_store
    this->message_store = message_store;

    // check if invalid message_store
    DIR* dir = opendir(this->message_store.c_str());
    if(dir == nullptr)
    {
        std::cerr << "Cannot access " << this->message_store << ": No such directory" << std::endl;
        exit(EXIT_FAILURE);
    }

    // closing directory
    closedir(dir);
}