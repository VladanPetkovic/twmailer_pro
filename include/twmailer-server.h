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
#include "ldap_fh.h"        // including costum ldap class


class MailServer {

public:
    MailServer(int port);
    ~MailServer();
    int getServerSocket();                                          // returns server_socket
    void setSocketOptions();                                        // sets socketoptions
    void handleClient(int client_socket);                           // processes user input: either SEND, DEL,...
    void logMessage(const std::string & msg);                       // Utility function to print logs with timestamp
    bool storeMessage(std::string receiver);                        // storing a message one user has send
    void checkDirectory(const std::string & message_store);         // checking if passed message_store exists

    enum LineBreakCount     // needed for finding sender, subject,... in this->buffer
    {                       // for example: receiver comes after 1 linebreak
        RECEIVER = 1,       // receiver after two,...
        SUBJECT = 2,
        MESSAGE = 3
    };

    struct SessionData 
    {
        std::string username;
        std::string ip;
    };

private:
    char* buffer;
    int server_socket = -1;
    int reuseValue = 1;
    struct sockaddr_in server_addr;
    std::string message_store;
    sem_t* semaphore;
    Ldap_fh ldap_server;
    
    void handleLogin(int client_socket, SessionData & sessionInfo); // process "LOGIN"
    void handleSend(int client_socket);                             // process "SEND"
    void handleList(int client_socket, SessionData & sessionInfo);  // process "LIST"
    void handleRead(int client_socket, SessionData & sessionInfo);  // process "READ"
    void handleDel(int client_socket, SessionData & sessionInfo);   // process "DEL"
    std::string getSenderOrReceiver(int startPosition);             // return Username of sender or receiver
    int getStartPosOfString(LineBreakCount count);                  // returning start-position of sender, receiver, subject,...
    int getEndPosOfMessage(int startOfMessage);                     // returns end-position of a message
    int getFileCount(std::string username);                         // counts files of submitted user
    int getMaxMessageNumber(std::string username);                  // returns the max. Msg-number of one user --> make unique msg-numbers
    int getMessageNumber(int startOfMessageNumber);                 // returns Msg-number, which was submitted by user, from buffer
    bool createAndWriteFile(std::string receiver);                  //creating and writing file
    std::string getClientIP(int client_socket);                     // returns client-ip
};

#endif // TWMAILER_SERVER_H
