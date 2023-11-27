#ifndef LDAP_FH_H
#define LDAP_FH_H

#include <iostream>         // in - and output (and debug-messages)
#include <fstream>          // for file operations
#include <string>           // easy string handling
#include <cstring>          // strcpy() strlen() ...
#include <ldap.h>           // LDAP-integration
#include <ctime>            // for printing time -- view Examples.md for possible errors
#include <semaphore.h>      // needed for synchronization of processes
#include <fcntl.h>          // needed for Semaphores

struct SessionData 
{
    std::string username;
    std::string ip;
};

class Ldap_fh {
public:
    Ldap_fh();
    Ldap_fh(sem_t* semaphore);
    virtual ~Ldap_fh();
    void logMessage(const std::string & msg);
    bool authenticateWithLdap(const std::string & username, const std::string & password);
    bool isUserBlacklisted(SessionData & sessionInfo);
    int getLoginAttempts(SessionData & sessionInfo);
    void updateLoginAttempt(SessionData & sessionInfo);
    void resetLoginAttempt(SessionData & sessionInfo);

private:
    sem_t* semaphore;
    std::string blacklist = "blacklist/blacklist.txt";
    std::string blacklist_log = "blacklist/blacklist_log.txt";
};

#endif // LDAP_FH_H