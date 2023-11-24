#include "../include/twmailer-client.h"

const int BUFFER_SIZE = 1024;

MailClient::MailClient(const std::string & ip, int port)
{
    // allocate memory for buffer
    this->buffer = new char[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Cannot create socket");
        throw std::runtime_error("Cannot create socket");
    }

    sockaddr_in server_addr;
    memset( & server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), & server_addr.sin_addr) <= 0)
    {
        close(client_socket);
        perror("inet_pton");
        throw std::runtime_error("Invalid IP address format");
    }

    if (connect(client_socket, (struct sockaddr * ) & server_addr, sizeof(server_addr)) == -1)
    {
        close(client_socket);
        perror("Connect error");
        throw std::runtime_error("Connect error");
    }

    std::cout << "Connected to server." << std::endl;
}
MailClient::~MailClient()
{
    // free allocated memory
    delete[] this->buffer;

    if (client_socket != -1)
    {
        close(client_socket);
    }
}
void MailClient::writeToSocket(const char* buffer, size_t size)
{
    // if message is longer than our buffer
    if(size > BUFFER_SIZE)
    {
        size = BUFFER_SIZE - 1;
    }
    // if sending fails
    if (send(client_socket, buffer, size, 0) == -1)
    {
        perror("Send error");
    }
}
void MailClient::readFromSocket()
{
    // resetting buffer
    memset(this->buffer, '\0', BUFFER_SIZE);

    int bytes_read = recv(client_socket, this->buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read == -1)
    {
        perror("recv error");
        exit(EXIT_FAILURE);
    }
    this->buffer[bytes_read] = '\0';  // Null-terminate buffer
}
void MailClient::run()
{
    std::string command;

    // Login process
    bool isLoggedIn = false;
    while (!isLoggedIn)
    {
        std::cout << "\nLogin with your credentials.\n";
        isLoggedIn = handleLogin();
        if (!isLoggedIn) {
            std::cout << "Login failed. Please try again.\n";
        }
    }
    std::cout << "Login successful.\n";

    // Main loop for mail application
    while (true)
    {
        std::cout << "\nEnter command (SEND, LIST, READ, DEL, QUIT): ";
        std::getline(std::cin, command);

        // Handle each command
        if (command == "SEND") {
            handleSend();
        }
        else if (command == "LIST") {
            handleList();
        }
        else if (command == "READ") {
            handleRead();
        }
        else if (command == "DEL") {
            handleDel();
        }
        else if (command == "QUIT") {
            break; // Exit the loop to end the program
        }
        else {
            std::cout << "Invalid command." << std::endl;
        }
    }
}
bool MailClient::handleLogin()
{
    std::string password;
    // INPUT
    std::cout << "\nUsername: ";
    std::cin >> this->username;
    std::cout << "Password: ";
    std::cin >> password;

    std::cin.ignore(); // Flush newline from buffer
    // if((!isValidUsername(this->username))) // NOT NEEDED
    // {
    //     std::cout   << "Username is not valid:" << std::endl
    //                 << "\tUse only lowercase letters (a-z) and/or digits (0-9)" << std::endl
    //                 << "\tMax. length: 8 chars!" << std::endl;
    //     return false;
    // }

    // SENDING
    strcpy(this->buffer, "LOGIN\n");
    strcat(this->buffer, this->username.c_str());
    strcat(this->buffer, "\n");
    strcat(this->buffer, password.c_str());
    strcat(this->buffer, "\n");
    writeToSocket(this->buffer, strlen(this->buffer) - 1);

    // READING
    readFromSocket();

    // RESPONSE
    std::string response = this->buffer;
    if(response == "OK\n")
    {
        std::cout << "OK\n";
        return true;
    }
    else
    {
        std::cout << "ERR\n";
        return false;
    }
}
void MailClient::handleSend()
{
    std::string sender, receiver, subject, message, send_buffer;

    // INPUT
    std::cout << "Sender: ";
    std::cin >> sender;
    std::cout << "Receiver: ";
    std::cin >> receiver;

    std::cin.ignore(); // Flush newline from buffer
    if((!isValidUsername(sender)) || (!isValidUsername(receiver)))  // checking input
    {
        std::cout   << "One of the usernames is not valid:" << std::endl
                    << "\tUse only lowercase letters (a-z) and/or digits (0-9)" << std::endl
                    << "\tMax. length: 8 chars!" << std::endl;
        return;
    }
    std::cout << "Subject (max 80 chars): ";
    std::getline(std::cin, subject);
    if(subject.length() > 80)
    {
        std::cout << "Subject too long! Max. length: 80 chars!" << std::endl;
        return;
    }

    std::cout << "Message (end with a single '.'): ";
    while (true)
    {
        std::string line;
        std::getline(std::cin, line);
        if (line == ".")
        {
            break;
        }
        message += line + "\n";
    }

    // SENDING --> not beautiful, but we do not want to use additional storage
    strcpy(this->buffer, "SEND\n");
    strcat(this->buffer, sender.c_str());
    strcat(this->buffer, "\n");
    strcat(this->buffer, receiver.c_str());
    strcat(this->buffer, "\n");
    strcat(this->buffer, subject.c_str());
    strcat(this->buffer, "\n");
    strcat(this->buffer, message.c_str());
    strcat(this->buffer, "\n.\n");
    writeToSocket(this->buffer, strlen(this->buffer) - 1);

    // READING
    readFromSocket();

    // RESPONSE
    std::string response = this->buffer;
    if(response == "OK\n")
    {
        std::cout << "OK\n";
    }
    else
    {
        std::cout << "ERR\n";
    }
}
void MailClient::handleList()
{
    std::string username;

    // INPUT
    std::cout << "Username: ";
    std::cin >> username;
    std::cin.ignore(); // Flush newline from buffer
    if(!isValidUsername(username))  // checks if username is valid
    {
        std::cout   << "One of the usernames is not valid:" << std::endl
                    << "\tUse only lowercase letters (a-z) and/or digits (0-9)" << std::endl
                    << "\tMax. length: 8 chars!" << std::endl;
        return;
    }

    // SENDING
    std::string send_buffer = "LIST\n" + username + "\n";
    writeToSocket(send_buffer.c_str(), send_buffer.size());

    // READING
    readFromSocket();

    // RESPONSE
    std::cout << this->buffer << std::endl;
}
void MailClient::handleRead()
{
    std::string username;
    int msg_num;
    std::string output[4] = {"Sender", "Receiver", "Subject", "Message"};
    int outputCount = 0;

    // INPUT
    std::cout << "Username: ";
    std::cin >> username;
    std::cout << "Message Number: ";
    if(!(std::cin >> msg_num))  // checks if string is submitted
    {
        std::cout << "Invalid input! Only digits (0-9)!" << std::endl;
        std::cin.clear();
        std::cin.ignore();
        return;
    }
    std::cin.ignore(); // Flush newline from buffer
    if(!isValidUsername(username))  // checks if username is valid
    {
        std::cout   << "One of the usernames is not valid:" << std::endl
                    << "\tUse only lowercase letters (a-z) and/or digits (0-9)" << std::endl
                    << "\tMax. length: 8 chars!" << std::endl;
        return;
    }

    // SENDING
    std::string send_buffer = "READ\n" + username + "\n" + std::to_string(msg_num) + "\n";
    writeToSocket(send_buffer.c_str(), send_buffer.size());

    // READING
    readFromSocket();

    // RESPONSE
    if(strncmp(this->buffer, "OK\n", 3) == 0)
    {
        for(int i = 0; buffer[i] != '\0'; i++)
        {
            std::cout << buffer[i];
            if(buffer[i] == '\n' && outputCount < 4)
            {
                std::cout << output[outputCount] << ": ";
                outputCount++;
            }
        }
    }
    else
    {
        std::cout << "ERR\n" << std::endl;
    }
}
void MailClient::handleDel()
{
    std::string username;
    int msg_num;

    // INPUT
    std::cout << "Username: ";
    std::cin >> username;
    std::cout << "Message Number: ";
    if(!(std::cin >> msg_num))  // checks if string is submitted
    {
        std::cout << "Invalid input! Only digits (0-9)!" << std::endl;
        std::cin.clear();
        std::cin.ignore();
        return;
    }
    std::cin.ignore(); // Flush newline from buffer
    if(!isValidUsername(username))  // checks if username is valid
    {
        std::cout   << "One of the usernames is not valid:" << std::endl
                    << "\tUse only lowercase letters (a-z) and/or digits (0-9)" << std::endl
                    << "\tMax. length: 8 chars!" << std::endl;
        return;
    }

    // SENDING
    std::string send_buffer = "DEL\n" + username + "\n" + std::to_string(msg_num) + "\n";
    writeToSocket(send_buffer.c_str(), send_buffer.size());

    // READING
    readFromSocket();

    // RESPONSE
    if (strncmp(this->buffer, "OK\n", 3) == 0)
    {
        std::cout << "OK\n";
    }
    else
    {
        std::cout << "ERR\n";
    }
}
bool MailClient::isValidUsername(const std::string & username)
{
    // username longer than 8 chars
    if(username.length() > 8)
    {
        return false;
    }

    // we are using the ascii values of digits (0-9) and lowercase letter (a-z)
    // digits: '0' = 48 and '9' = 57
    // lowercase letters: 'a' = 97 and 'z' = 122
    for(int i : username)
    {
        // if a char is not in one of the two ranges, we return false
        if(!((i <= 57 && i >= 48) || (i <= 122 && i >= 97)))
        {
            return false;
        }
    }
    return true;
}