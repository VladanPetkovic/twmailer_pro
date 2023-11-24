#ifndef TWMAILER_CLIENT_H
#define TWMAILER_CLIENT_H

#include <iostream>         // in - and output (and debug-messages)
#include <string>           // easy string handling
#include <cstring>          // strcpy() strlen() ...
#include <sys/socket.h>     // socket functions
#include <cerrno>           // errno variables 
#include <arpa/inet.h>      // for ip addresses
#include <unistd.h>         // read() write() close() ...

class MailClient {
public:
    MailClient(const std::string& ip, int port);
    ~MailClient();
    void run();  
                                               // disponent for SEND, LIST, READ, DEL

private:
    char* buffer;
    int client_socket;
    std::string username;

    void writeToSocket(const char* buffer, size_t size);    // writing to socket
    void readFromSocket();                                  // reading from socket
    bool handleLogin();
    void handleSend();                                      // process "SEND"
    void handleList();                                      // process "LIST"
    void handleRead();                                      // process "READ"
    void handleDel();                                       // process "DEL"
    bool isValidUsername(const std::string & username);     // returns if username is valid --> fun with ascii representation
};

#endif // TWMAILER_CLIENT_H
