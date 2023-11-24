#include "../include/twmailer-server.h"

int main(int argc, char * argv[])
{
    /****************************************************************************************/
    // not enough arguments passed

    // // Initialize semaphore
    // sem_t *semaphore = sem_open("/twmailerServerSemaphore", O_CREAT, 0644, 1);
    // if (semaphore == SEM_FAILED) {
    //     perror("Error initializing semaphore");
    //     exit(EXIT_FAILURE);
    // }

    if (argc != 3)
    {
        std::cerr << "Usage: ./twmailer-server <port> <mail-spool-directoryname>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);

    /****************************************************************************************/
    // starting server
    MailServer server = MailServer(port);
    // checking existance of passed message folder
    server.checkDirectory(argv[2]);
    // displaying first message
    server.logMessage("Waiting for connections...");

    /****************************************************************************************/
    std::vector < pid_t > children;
    // Handle user input, send commands to the server, and display server responses
    while (true)
    {   
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server.getServerSocket(), (struct sockaddr * ) & client_addr, & client_len);
        if (client_socket == -1)
        {
            perror("Failed to accept client connection.");
            continue; // go to the next iteration
        }

        // after receiving successful socket connection --> forking
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            close(client_socket); // Close socket on fork error
            continue;
        }
        
        if (pid > 0) 
        {
            close(client_socket);
            // parent-process: adding processID to vector children
            children.push_back(pid);
        }
        else
        {   
            // sem_wait(semaphore); // Acquire semaphore

            server.logMessage("Client connected.");
            server.handleClient(client_socket);
            server.logMessage("Closing client connection.");

            // sem_post(semaphore); // Release semaphore
            close(client_socket);
            exit(EXIT_SUCCESS);
        }
    }

    // parent-process waits for children-process to finish
    for (const pid_t & child: children) {
        waitpid(child, nullptr, 0);
    }
    close(server.getServerSocket());

    // // Clean semaphores
    // sem_close(semaphore);
    // sem_unlink("/twmailerServerSemaphore");

    return 0;
}