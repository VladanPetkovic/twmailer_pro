#ifndef LDAP_FH_H
#define LDAP_FH_H

#include <iostream>         // in - and output (and debug-messages)
#include <fstream>          // for file operations
#include <string>           // easy string handling
#include <cstring>          // strcpy() strlen() ...
#include <ldap.h>           // LDAP-integration
#include <ctime>            // for printing time -- view Examples.md for possible errors
#include <sstream>          // for converting string to time_t
#include <semaphore.h>      // needed for synchronization of processes
#include <fcntl.h>          // needed for Semaphores

class Ldap_fh {
public:
    Ldap_fh();
    Ldap_fh(sem_t* semaphore);
    virtual ~Ldap_fh();
    void logMessage(const std::string & msg);
    bool authenticateWithLdap(const std::string & username, const std::string & password);
    bool isUserBlacklisted(const std::string & username, const std::string & ip);
    int getLoginAttempts(const std::string & username, const std::string & ip);
    time_t getAttemptTime(const std::string & username, const std::string & ip);
    void writeNewUserInBlacklist(const std::string & username, const std::string & ip);
    void updateLoginAttempt(const std::string & username, const std::string & ip, bool deleteUser);
    bool isUserInBlacklist(const std::string & username, const std::string & ip);

private:
    sem_t* semaphore;
    std::string blacklist = "blacklist/blacklist";
};

#endif // LDAP_FH_H