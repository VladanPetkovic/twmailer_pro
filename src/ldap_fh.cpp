#include "../include/ldap_fh.h"

Ldap_fh::Ldap_fh()
{

}
Ldap_fh::Ldap_fh(sem_t* semaphore)
{
    this->semaphore = semaphore;
}
Ldap_fh::~Ldap_fh()
{

}
void Ldap_fh::logMessage(const std::string & msg = "")
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
bool Ldap_fh::authenticateWithLdap(const std::string& username, const std::string& password)
{
    // Using provided source code from moodle

    // LDAP config
    // anonymous bind with user and pw empty
    const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3;
    LDAP *ldapHandle;
    LDAPMessage *searchResult = nullptr;
    int rc = 0;
    char ldapBindUser[256];
    sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", username.c_str());

    try
    {
        // Initialize LDAP connection
        rc = ldap_initialize(&ldapHandle, ldapUri);
        if (rc != LDAP_SUCCESS)
        {
            logMessage("LDAP connection failed.");
            throw std::runtime_error("LDAP connection initialization failed");
        }

        ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);

        if (rc != LDAP_SUCCESS)
        {
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            throw std::runtime_error("ldap_set_option failed");
        }

        rc = ldap_start_tls_s(
            ldapHandle,
            NULL,
            NULL);
        if (rc != LDAP_SUCCESS)
        {
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            throw std::runtime_error("ldap_start_tls_s failed");
        }

        // search settings
        const char *ldapSearchBaseDomainComponent = "dc=technikum-wien,dc=at";
        const char *ldapSearchFilter = ("(uid=" + username + ")").c_str();
        ber_int_t ldapSearchScope = LDAP_SCOPE_SUBTREE;
        const char *ldapSearchResultAttributes[] = {"uid", "cn", NULL};

        BerValue bindCredentials;
        bindCredentials.bv_val = (char *)password.c_str();
        bindCredentials.bv_len = strlen(password.c_str());
        BerValue *servercredp; // server's credentials
        rc = ldap_sasl_bind_s(
            ldapHandle,
            ldapBindUser,
            LDAP_SASL_SIMPLE,
            &bindCredentials,
            NULL,
            NULL,
            &servercredp);
        if (rc != LDAP_SUCCESS)
        {
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            throw std::runtime_error("ldap_sasl_bind_s failed");
        }

        LDAPMessage *searchResult;
        rc = ldap_search_ext_s(
            ldapHandle,
            ldapSearchBaseDomainComponent,
            ldapSearchScope,
            ldapSearchFilter,
            (char **)ldapSearchResultAttributes,
            0,
            NULL,
            NULL,
            NULL,
            500,
            &searchResult);
        if (rc != LDAP_SUCCESS)
        {
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            throw std::runtime_error("ldap_search_ext_s failed");
        }

        // Successful authentication
        return true;
    }
    catch (const std::exception& e)
    {
        logMessage(std::string("LDAP authentication error: ") + e.what());
        return false;
    }
    
    // Free LDAP resources in all scenarios
    if(searchResult)
    {
        ldap_msgfree(searchResult);
    } 
    if(ldapHandle)
    {
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
    }
    return false;
}
bool Ldap_fh::isUserBlacklisted(const std::string & username, const std::string & ip)
{
    // checks and updates the blacklist
    if(!isUserInBlacklist(username, ip))
    {
        return false;
    } 
    else
    {
        time_t attemptTime = getAttemptTime(username, ip);
        
        if(difftime(std::time(nullptr), attemptTime) >= 60)
        {
            updateLoginAttempt(username, ip, true);
            return false;
        }
        else if(getAttemptTime(username, ip) == 3 && difftime(std::time(nullptr), attemptTime) < 60)
        {
            return true;
        }
    }

    return false;
}
int Ldap_fh::getLoginAttempts(const std::string & username, const std::string & ip)
{
    int attempts = 0;
    char input;
    std::string line;
    // read file_blacklist
    std::ifstream file_blacklist(this->blacklist);

    // opening and writing file
    if(file_blacklist.is_open())
    {
        // search for username and ip
        std::streampos positionTemp = 0;
        while(getline(file_blacklist, line))
        {
            positionTemp = file_blacklist.tellg();
            if(line.find(username + "," + ip) != std::string::npos)
            {
                break;
            }
        }
        
        // closing blacklist
        file_blacklist.close();
    }
    else
    {
        std::cerr << "Failed to open the file." << std::endl;
    }

    // convert string line to int attempts
    size_t position = line.find(':');
    position++;
    input = line[position];
    attempts = input - '0';

    return attempts;
}
time_t Ldap_fh::getAttemptTime(const std::string & username, const std::string & ip)
{
    time_t attemptTime;
    std::string line;
    std::string input;
    // read file_blacklist
    std::ifstream file_blacklist(this->blacklist);

    // opening and writing file
    if(file_blacklist.is_open())
    {
        // search for username and ip
        std::streampos positionTemp = 0;
        while(getline(file_blacklist, line))
        {
            positionTemp = file_blacklist.tellg();
            if(line.find(username + "," + ip) != std::string::npos)
            {
                break;
            }
        }
        
        // closing blacklist
        file_blacklist.close();
    }
    else
    {
        std::cerr << "Failed to open the file." << std::endl;
    }

    // convert String line to time_t
    size_t position = line.find(':');
    position += 3;
    input = line.substr(position, 10);  // length of time-string == 10
    int seconds;
    std::istringstream secondsStream(input);
    secondsStream >> seconds;
    attemptTime = static_cast<time_t>(seconds);

    return attemptTime;
}
void Ldap_fh::writeNewUserInBlacklist(const std::string & username, const std::string & ip)
{
    // lock semaphore: files are being edited
    sem_wait(this->semaphore);

    // append new entry in file_blacklist
    std::ofstream file_blacklist(this->blacklist, std::ios::app);

    // opening and writing file
    if(file_blacklist.is_open())
    {
        // write username, ip, time, loginattempts, if not already in blacklist
        file_blacklist << username + "," 
                    + ip + ",:"
                    + std::to_string(1) + ","
                    + std::to_string(std::time(nullptr)) + "\n";
        
        // closing blacklist
        file_blacklist.close();
    }
    else
    {
        std::cerr << "Failed to open the file." << std::endl;
    }

    // unlock semaphore
    sem_post(this->semaphore);
}
void Ldap_fh::updateLoginAttempt(const std::string & username, const std::string & ip, bool deleteUser)
{
    // lock semaphore: files are being edited
    sem_wait(this->semaphore);

    int attempts = getLoginAttempts(username, ip);
    time_t attemptTime = getAttemptTime(username, ip);
    // read from file_blacklist
    std::ifstream file_blacklist(this->blacklist);
    // write in new file
    std::ofstream file_temp("blacklist/temp");
    std::string line;
    std::size_t positionString;

    // calculating attempts
    if(attempts < 3)
    {
        attempts++;         // user gets new warnings
    }
    else if(difftime(std::time(nullptr), attemptTime) >= 60 && attempts == 3)
    {
        attempts = 1;       // user is not more blacklisted
        deleteUser = true;  // delete user entirely from blacklist
    }

    // reading old file and writing new file with updated information
    while(getline(file_blacklist, line))
    {
        positionString = line.find(username + "," + ip);
        if(positionString == std::string::npos)     // current line does not match the user line
        {
            file_temp << line + "\n";

        }
        else if(!deleteUser && positionString != std::string::npos) // write only, if deleteUser == false
        {
            file_temp << username + "," 
                    + ip + ",:"
                    + std::to_string(attempts) + ","
                    + std::to_string(std::time(nullptr)) + "\n";
        }
    }

    // closing files
    file_blacklist.close();
    file_temp.close();

    // removing old blacklist
    remove(this->blacklist.c_str());
    // renaming new blacklist
    rename("blacklist/temp", this->blacklist.c_str());

    // unlock semaphore
    sem_post(this->semaphore);
}
bool Ldap_fh::isUserInBlacklist(const std::string & username, const std::string & ip)
{
    std::ifstream file_blacklist(this->blacklist);
    std::string line;
    std::size_t positionString;

    // opening and writing file
    while(getline(file_blacklist, line))
    {
        positionString = line.find(username + "," + ip);
        if(positionString != std::string::npos)     // current line matches with username and ip
        {
            // closing files
            file_blacklist.close();
            return true;
        }
    }

    return false;
}