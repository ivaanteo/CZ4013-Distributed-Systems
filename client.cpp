#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "./socket.cpp"
#include <string>

int main() {
    
    // Create a socket
    Socket serverSocket(AF_INET, SOCK_STREAM, 0);

    // Connect to the server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Server port
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // Server IP address (localhost)
    serverSocket.connect((sockaddr*)&serverAddr, sizeof(serverAddr));
    
    // Create buffer
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    while (true) {
        // Get user input
        std::string message;
        std::cout << "Enter a message: \n";
        std::getline(std::cin, message);
        const char* msg = &message[0];

        // Send data to server
        serverSocket.send(msg, strlen(msg), 0);

        // Receive data from server
        int bytesReceived = serverSocket.recv(buffer, sizeof(buffer), 0);
        std::cout << "Received from server: " << buffer << std::endl;
    }
    
    // Close socket
    close(serverSocket);

    return 0;
}
