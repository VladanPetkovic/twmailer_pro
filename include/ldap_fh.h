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
    bool authenticateWithLdap(const std::string& username, const std::string& password);

private:
    sem_t* semaphore;
    
};

#endif // LDAP_FH_H