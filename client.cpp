#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "./socket.cpp"
#include <string>

class Client {
public:
    Client() {
        // Create a socket
        clientSocket = new Socket(AF_INET, SOCK_STREAM, 0);

        // Create server address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8080); // Server port
        inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // Server IP address (localhost)
    }

    void connect() {
        // Connect to the server
        clientSocket->connect((sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    void run() {
        // Create buffer
        char buffer[1024];
        
        while (true) {
            // Reset buffer
            memset(buffer, 0, sizeof(buffer));

            // Receive data from server -- Receive intro message first
            int bytesReceived = clientSocket->recv(buffer, sizeof(buffer), 0);
            std::cout << "Received from server: " << buffer << std::endl;

            // Get user input
            std::string msg;
            getUserInput("Enter a message: ", msg);
            handleUserInput(&msg[0]);

            // Send data to server -- to be removed and put everything in handleUserInput
            clientSocket->send(&msg[0], strlen(&msg[0]), 0);
        }
    }

    ~Client() {
        delete clientSocket;
    }

private:
    Socket* clientSocket;
    sockaddr_in serverAddr;
    char* buffer[1024];

    // Client helper functions

    void getUserInput(std::string message, std::string& input) {
        std::cout << message << std::endl;
        std::getline(std::cin, input);
    }

    void handleSubscribe() {

        std::string fileName;
        getUserInput("Enter the file you would like to subscribe to: ", fileName);

        std::string duration;
        getUserInput("Enter the duration you would like to subscribe for (in seconds): ", duration);

        std::string timestamp = std::to_string(time(0) + std::stoi(duration));
        
        // // Send subscribe message to server
        std::string request = "subscribe " + fileName + " " + timestamp;
        clientSocket->send(&request[0], strlen(&request[0]), 0);

        // Receive response on whether subscription was successful
        clientSocket->recv(buffer, sizeof(buffer), 0);

        // If successful, wait for updates

        // Unsubscribe after duration
        
    }

    void handleUserInput(char* input) {
        if (strcmp(input, "exit") == 0){
            std::cout << "Exiting...\n";
            exit(0);
        }
        if (strcmp(input, "help") == 0){
            std::cout << "Commands: \n";
            std::cout << "exit - Exit the program\n";
            std::cout << "help - Display this message\n";
            std::cout << "subscribe - Subscribe to updates for a file\n";
            return;
        }
        if (strcmp(input, "subscribe") == 0){
            handleSubscribe();
            return;
        }

        // TODO: Add more features here

        std::cout << "Invalid command. Type 'help' for a list of commands\n";   
    }
    
};

int main() {
    
    Client client;

    client.connect();

    client.run();

    return 0;
}


