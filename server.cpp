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
        serverSocket = new Socket(AF_INET, SOCK_STREAM, 0);
        
        // Bind the socket to an IP address and port
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(port); // Server port
        serverSocket->bind((sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // Listen for incoming connections
        serverSocket->listen(10);
        
        // Accept a client connection
        clientAddrSize = sizeof(clientAddr);
        clientSocketDescriptor = serverSocket->accept((sockaddr*)&clientAddr, &clientAddrSize);
        clientSocket = new Socket(clientSocketDescriptor);
    }
    
    void run() {
        // Send intro message
        const char* introMessage = "Welcome to the server!";
        clientSocket->send(introMessage, strlen(introMessage), 0);
        
        // Receive data from client and echo it back
        char buffer[1024];
        while (true) {
            // Reset buffer
            memset(buffer, 0, sizeof(buffer));
            
            int bytesReceived = clientSocket->recv(buffer, sizeof(buffer), 0);
            std::cout << "Received from client: " << buffer << std::endl;
            
            // If input is "subscribe", wait 10 seconds then send a message
            if (strcmp(buffer, "subscribe") == 0) {
                sleep(10); // Simulate waiting for data
                const char* message = "You are now subscribed!"; // Simulate updated data
                clientSocket->send(message, strlen(message), 0);
                continue;
            } 
            
            // Send
            clientSocket->send(buffer, bytesReceived, 0);
        }
    }
    
    ~Server() {
        delete serverSocket;
        delete clientSocket;
    }

private:
    Socket* serverSocket;
    Socket* clientSocket;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;
    socklen_t clientAddrSize;
    int clientSocketDescriptor;
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
