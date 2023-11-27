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
    return false;
}
int Ldap_fh::getLoginAttempts(const std::string & username, const std::string & ip)
{

}
void Ldap_fh::updateLoginAttempt(const std::string & username, const std::string & ip)
{
    int attempt;
    bool userFound = false;
    // attempt.ip = ip;

    // if (difftime(std::time(nullptr), attempt.lastAttemptTime) > 60)
    // {
    //     attempt.attempts = 1;
    // } 
    // else 
    // {
    //     attempt.attempts++;
    // }
    // attempt.lastAttemptTime = std::time(nullptr);

    // read and write in file_blacklist
    std::fstream file_blacklist(this->blacklist, std::ios::in | std::ios::out);
    // only write in our logfile
    std::ofstream file_blacklist_log(this->blacklist_log);

    // opening and writing file
    if(file_blacklist.is_open())
    {
        // search for username and ip
        std::string line;
        std::streampos position = 0;
        while (getline(file_blacklist, line))
        {
            position = file_blacklist.tellg();
            if (line.find(username + "," + ip) != std::string::npos)
            {
                userFound = true;
                break;
            }
        }

        // moving the cursor to the position
        file_blacklist.seekp(position);

        // write username, ip, time, loginattempts
        file_blacklist << username + "," 
                        + ip + "," 
                        + std::to_string(std::time(nullptr)) + ","
                        + ;

        // closing blacklist
        file_blacklist.close();
    }
    else
    {
        std::cerr << "Failed to open the file." << std::endl;
    }
}
void Ldap_fh::resetLoginAttempt(const std::string & username, const std::string & ip)
{
    
}