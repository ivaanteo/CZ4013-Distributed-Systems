#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[BUFFER_SIZE];
    int len, n;

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        std::cout << "Error: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    len = sizeof(cliaddr);
    // n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
    n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, (socklen_t *)&len);
    buffer[n] = '\0';

    printf("Received message: %s\n", buffer);

    // Convert IP address to a human-readable string
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, INET_ADDRSTRLEN);

    printf("Sender IP address: %s\n", ip);

    // close(sockfd);
    return 0;
}
