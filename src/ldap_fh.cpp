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
bool Ldap_fh::isUserBlacklisted(const std::string & username, const std::string & ip)
{
    if(!isUserInBlacklist(username, ip))
    {
        return false;
    } 
    else
    {
        time_t attemptTime = getAttemptTime(username, ip);
        
        if(getAttemptTime(username, ip) == 3 && difftime(std::time(nullptr), attemptTime) >= 60)
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
    int prefixLength = username.length() + ip.length() + 3; // adding 3 for "," and ",:"
    int attempts = 0;
    // read file_blacklist
    std::ifstream file_blacklist(this->blacklist, std::ios::in | std::ios::out);

    // opening and writing file
    if(file_blacklist.is_open())
    {
        // search for username and ip
        std::string line;
        std::streampos positionTemp = 0;
        std::size_t positionString;
        while(getline(file_blacklist, line))
        {
            positionTemp = file_blacklist.tellg();
            positionString = line.find(username + "," + ip);
            if(positionString != std::string::npos)
            {
                file_blacklist.seekg(positionString + prefixLength, std::ios::beg);
                // saving the number of attempts
                file_blacklist >> attempts;
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

    return attempts;
}
time_t Ldap_fh::getAttemptTime(const std::string & username, const std::string & ip)
{
    int prefixLength = username.length() + ip.length() + 5; // adding 5 for "," and ",:" and "," and the attemptnumber
    time_t attemptTime;
    // read file_blacklist
    std::ifstream file_blacklist(this->blacklist, std::ios::in | std::ios::out);

    // opening and writing file
    if(file_blacklist.is_open())
    {
        // search for username and ip
        std::string line;
        std::streampos positionTemp = 0;
        std::size_t positionString;
        while(getline(file_blacklist, line))
        {
            positionTemp = file_blacklist.tellg();
            positionString = line.find(username + "," + ip);
            if(positionString != std::string::npos)
            {
                file_blacklist.seekg(positionString + prefixLength, std::ios::beg);
                // saving the number of attempts
                file_blacklist >> attemptTime;
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

    return attemptTime;
}
void Ldap_fh::writeNewUserInBlacklist(const std::string & username, const std::string & ip)
{
    // read and write in file_blacklist
    std::fstream file_blacklist(this->blacklist, std::ios::in | std::ios::out);

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
}
void Ldap_fh::updateLoginAttempt(const std::string & username, const std::string & ip, bool deleteUser = false)
{
    int attempts = getLoginAttempts(username, ip);
    time_t attemptTime = getAttemptTime(username, ip);
    // read from file_blacklist
    std::ifstream file_blacklist(this->blacklist);
    // write in new file
    std::ofstream file_temp("blacklist/temp.txt");
    std::string line;
    std::size_t positionString;

    // calculating attempts
    if(attempts < 3)
    {
        attempts++;         // user gets new warnings
    }
    else if(difftime(std::time(nullptr), attemptTime) >= 60 && attempts == 3)
    {
        attempts = 0;       // user is not more blacklisted
        deleteUser = true;  // delete user entirely from blacklist
    }

    // reading old file and writing new file with updated information
    while(getline(file_blacklist, line))
    {
        positionString = line.find(username + "," + ip);
        if(positionString == std::string::npos)     // current line does not match the user line
        {
            file_temp << line;
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
    rename("temp.txt", this->blacklist.c_str());
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
            return true;
        }
    }

    return false;
}