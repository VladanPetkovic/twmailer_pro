#include "../include/twmailer-client.h"

int main(int argc, char * argv[])
{
    // not enough arguments passed
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return 1;
    }

    // starting client
    try
    {
        MailClient client(argv[1], std::atoi(argv[2]));
        client.run();
    } 
    catch (const std::exception & e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}