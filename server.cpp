#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "./socket.cpp"


int main(int argc, char* argv[]) {
    // Create a socket
    Socket serverSocket(AF_INET, SOCK_STREAM, 0);
    
    // Bind the socket to an IP address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
    serverAddr.sin_port = htons(8080); // Use port 8080
    serverSocket.bind((sockaddr*)&serverAddr, sizeof(serverAddr));


    // Listen for incoming connections
    serverSocket.listen(10);

    // Accept a client connection
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientSocketDescriptor = serverSocket.accept((sockaddr*)&clientAddr, &clientAddrSize);
    Socket clientSocket(clientSocketDescriptor);
    std::cout << "Client connected\n";

    // Send intro message
    const char* introMessage = "Welcome to the server!";
    if (clientSocket.send(introMessage, strlen(introMessage), 0) == -1) {
        std::cerr << "Error: Could not send data to client\n";
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    // Receive data from client and echo it back
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        
        int bytesReceived = clientSocket.recv(buffer, sizeof(buffer), 0);
        if (bytesReceived == -1) {
            std::cerr << "Error: Could not receive data from client\n";
            close(clientSocket);
            close(serverSocket);
            return 1;
        }
        if (bytesReceived == 0) {
            std::cout << "Client disconnected\n";
            break;
        }
        std::cout << "Received from client: " << buffer << std::endl;
        
        // Send
        if (clientSocket.send(buffer, bytesReceived, 0) == -1) {
            std::cerr << "Error: Could not send data to client\n";
            close(clientSocket);
            close(serverSocket);
            return 1;
        }
    }

    // Close sockets
    close(clientSocketDescriptor);
    close(serverSocket);

    return 0;
}
