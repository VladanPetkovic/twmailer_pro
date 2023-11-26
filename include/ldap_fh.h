#ifndef LDAP_FH_H
#define LDAP_FH_H

#include <iostream>         // in - and output (and debug-messages)
#include <string>           // easy string handling
#include <cstring>          // strcpy() strlen() ...
#include <ldap.h>           // LDAP-integration
#include <ctime>            // for printing time -- view Examples.md for possible errors
#include <semaphore.h>      // needed for synchronization of processes
#include <fcntl.h>          // needed for Semaphores


class Ldap_fh {
public:
    Ldap_fh();
    virtual ~Ldap_fh();
    void logMessage(const std::string & msg);
    bool authenticateWithLdap(const std::string & username, const std::string & password);
    bool isUserBlacklisted(const std::string & username, const std::string & ip);
    int getLoginAttempts(const std::string & username, const std::string & ip);
    void updateLoginAttempt(const std::string & username, const std::string & ip);
    void resetLoginAttempt(const std::string & username, const std::string & ip);

    struct LoginAttempt 
    {
        std::string ip;
        int attempts;
        std::time_t lastAttemptTime;
    };

private:
    std::string username;
    std::string ip;
    sem_t* semaphore;
    std::string blacklist = "blacklist/blacklist.txt";
    std::string blacklist_log = "blacklist/blacklist_log.txt";
    
};

#endif // LDAP_FH_H