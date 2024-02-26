#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "./socket.cpp"

class Server {
public:
    Server(int port) {
        // Create a socket
        serverSocket = new Socket(AF_INET, SOCK_DGRAM, 0);
        
        // Bind the socket to an IP address and port
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(port); // Server port
        serverSocket->bind((sockaddr*)&serverAddr, sizeof(serverAddr));

        std::cout << "Server listening on port " << port << std::endl;
    }
    
    void run() {
        // Receive data from client and echo it back
        while (true) {
            // Reset buffer
            memset(buffer, 0, sizeof(buffer));

            // Receive data from client
            ssize_t bytesReceived = serverSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrSize);
            std::cout << "Received from client: " << buffer << std::endl;
            clientAddr.sin_port = htons(8082);

            // echo to the same client
            ssize_t bytesSent = serverSocket->send(buffer, bytesReceived, 0, (sockaddr*)&clientAddr,  sizeof(clientAddr));

            // Handle request
            // handleRequest(bytesReceived);
            // receiveRequest();
        }
    }

    ~Server() {
        delete serverSocket;
    }

private:
    Socket* serverSocket;
    Socket* clientSocket;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;
    socklen_t clientAddrSize;
    int clientSocketDescriptor;
    char buffer[1024];
    // Subscriptions -- Filename : [Pair(timestamp, IP Address:Port), ...]
    // std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> subscriptions;

    // Server helper functions
    // Handle request

    int receiveRequest() {
        // Receive data from client
        ssize_t bytesReceived = serverSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrSize);
        if (bytesReceived == -1) {
            perror("Error: Could not receive data from client\n");
            return -1;
        }
        std::cout << "Received from client: " << buffer << std::endl;
        return bytesReceived;
    }

    void handleRequest(int bytesReceived) {
        // Here, respond to each command

        // If input is "subscribe", wait 10 seconds then send a message
        // Retrieve first token
        char* token = strtok(buffer, " ");
        if (strcmp(token, "subscribe") == 0) {
            handleSubscribe(token);
            return;
        }

        // Send data to client
        serverSocket->send(buffer, bytesReceived, 0, (sockaddr*)&clientAddr, clientAddrSize);
    }

    void handleSubscribe(char* token) {
        // Simulate waiting for data
        std::cout << "Subscribing..." << std::endl;

        // Print buffer
        std::cout << "Buffer: " << buffer << std::endl;
        // Split the message into tokens
        token = strtok(NULL, " ");
        std::string fileName = token;
        std::cout << "File: " << fileName << std::endl;
        token = strtok(NULL, " ");
        std::string timestamp = token;
        std::cout << "Timestamp: " << timestamp << std::endl;
        token = strtok(NULL, " ");
        std::string ipAddress = token;
        std::cout << "IP Address: " << ipAddress << std::endl;
        token = strtok(NULL, " ");
        std::string port = token;
        std::cout << "Port: " << port << std::endl;

        // Check validity of timestamp
        if (std::stoi(timestamp) < time(0)) {
            std::cerr << "Error: Invalid timestamp" << std::endl;
            return;
        }

        // Check validaity of file name
        if (fileName != "file1" && fileName != "file2") {
            std::cerr << "Error: Invalid file name" << std::endl;
            return;
        }

        // // Check if file is already subscribed by the client
        // for (auto subscription : subscriptions[fileName]) {
        //     if (subscription.second == ipAddress + ":" + port) {
        //         std::cerr << "Error: File already subscribed" << std::endl;
        //         return;
        //     }
        // }

        // Save the subscription
        // subscriptions[fileName].push_back(std::make_pair(std::stoi(timestamp), ipAddress + ":" + port));

        // Send message to client
        const char* message = "You are now subscribed!"; // Simulate updated data
        serverSocket->send(message, strlen(message), 0, (sockaddr*)&clientAddr, clientAddrSize);
        std::cout << "Sent to client: " << message << std::endl;
    }
};


int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);
    Server server(port);
    server.run();

    return 0;
}
