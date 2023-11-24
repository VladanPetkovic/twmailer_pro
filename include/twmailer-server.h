#ifndef TWMAILER_SERVER_H
#define TWMAILER_SERVER_H

#include <iostream>         // in - and output (and debug-messages)
#include <vector>           // using vectors
#include <string>           // easy string handling
#include <fstream>          // for file operations
#include <dirent.h>         // for directory operations
#include <sys/stat.h>       // for directory operations
#include <sys/socket.h>     // socket functions
#include <netinet/in.h>     // network communication
#include <arpa/inet.h>      // for ip addresses
#include <unistd.h>         // read() write() close() ...
#include <cstring>          // strcpy() strlen() ...
#include <ctime>            // for printing time -- view Examples.md for possible errors
#include <semaphore.h>      // needed for synchronization of processes
#include <sys/wait.h>       // fork()-call
#include <sys/types.h>      // fork()-call
#include <fcntl.h>          // needed for Semaphores
#include <ldap.h>           // LDAP-integration
#include <map>


class MailServer {

    enum LineBreakCount     // needed for finding sender, subject,... in this->buffer
    {
        SENDER = 1,         // for example: sender comes after 1 linebreak
        RECEIVER = 2,       // receiver after two,...
        SUBJECT = 3,
        MESSAGE = 4
    };

public:
    MailServer(int port);
    ~MailServer();
    int getServerSocket();                                          // returns server_socket
    void setSocketOptions();                                        // sets socketoptions
    void handleClient(int client_socket);                           // processes user input: either SEND, DEL,...
    void logMessage(const std::string & msg);                       // Utility function to print logs with timestamp
    bool storeMessage(std::string sender, std::string receiver);    // storing a message one user has send
    void checkDirectory(const std::string & message_store);         // checking if passed message_store exists

    struct LoginAttempt 
    {
        std::string ip;
        int attempts;
        std::time_t lastAttemptTime;
    };

private:
    char* buffer;
    int server_socket = -1;
    int reuseValue = 1;
    struct sockaddr_in server_addr;
    std::string message_store;
    sem_t* semaphore;
    
    void handleLogin(int client_socket);
    void handleSend(int client_socket);                             // process "SEND"
    void handleList(int client_socket);                             // process "LIST"
    void handleRead(int client_socket);                             // process "READ"
    void handleDel(int client_socket);                              // process "DEL"
    std::string getSenderOrReceiver(int startPosition);             // return Username of sender or receiver
    int getStartPosOfString(LineBreakCount count);                  // returning start-position of sender, receiver, subject,...
    int getEndPosOfMessage(int startOfMessage);                     // returns end-position of a message
    int getFileCount(std::string username);                         // counts files of submitted user
    int getMaxMessageNumber(std::string username);                  // returns the max. Msg-number of one user --> make unique msg-numbers
    int getMessageNumber(int startOfMessageNumber);                 // returns Msg-number, which was submitted by user, from buffer
    bool createAndWriteFile(std::string sender, std::string receiver);  //creating and writing file
    bool authenticateWithLDAP(const std::string& username, const std::string& password);
    void updateLoginAttempt(const std::string& username, const std::string& ip);
    std::string getClientIP(int client_socket);
    bool isUserBlacklisted(const std::string& username, const std::string& ip);
    void resetLoginAttempt(const std::string& username);
    void sendErrorResponse(int client_socket, const std::string& message);
    std::map<std::string, LoginAttempt> loginAttempts;
};

#endif // TWMAILER_SERVER_H
