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
        clientSocket = new Socket(AF_INET, SOCK_DGRAM, 0);

        // Create server address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(8081); // Server port

        // Set client address and port
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = INADDR_ANY; // Allow OS to assign the client IP
        clientAddr.sin_port = htons(8082); // Set client port

        clientSocket->bind((sockaddr*)&clientAddr, sizeof(clientAddr));
        std::cout << "Client bound to port "<< std::endl;
    }

    void run() {
        
        while (true) {
            // Get user input
            std::string msg;
            getUserInput("Enter a message: ", msg);
            handleUserInput(&msg[0]);
        }
    }

    ~Client() {
        delete clientSocket;
    }

private:
    Socket* clientSocket;
    sockaddr_in clientAddr;
    sockaddr_in serverAddr;
    socklen_t serverAddrSize;
    
    char buffer[1024];

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

        std::string ipAddress = clientSocket->getIP();
        std::string port = std::to_string(clientSocket->getPort());

        // // Send subscribe message to server
        std::string request = "subscribe " + fileName + " " + timestamp + " " + ipAddress + " " + port;
        sendRequest(request);


        // // Receive response on whether subscription was successful
        // clientSocket->recv(buffer, sizeof(buffer), 0);

        // memset(buffer, 0, sizeof(buffer)); // Temporarily clear buffer

        // If successful, wait for updates


        // Unsubscribe after duration
        
    }

    void sendRequest(const std::string& message) {
        clientSocket->send(message.c_str(), message.length(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        ssize_t bytesReceived = clientSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, &serverAddrSize);
        std::cout << "Received from client: " << buffer << std::endl;
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


        // Echo input
        std::string msg = std::string(input) + "\n";
        sendRequest(msg);
        // TODO: Add more features here
        std::cout << "Invalid command. Type 'help' for a list of commands\n";   
    }
    
};

int main() {
    
    Client client;

    client.run();

    return 0;
}


