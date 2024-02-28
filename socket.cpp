#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <arpa/inet.h>


// Wrapper class for a socket
class Socket {
public:
    Socket(int domain, int type, int protocol) {
        m_socket = socket(domain, type, protocol);
        if (m_socket == -1) {
            perror("Error: Could not create socket\n");
            exit(1);
        }
    }

    Socket(int socketDescriptor) { 
        m_socket = socketDescriptor;
    }

    ~Socket() {
        close(m_socket);
    }

    operator int() const {
        return m_socket;
    }

    int get() const {
        return m_socket;
    }

    // Get connections
    std::vector<int> getConnections() {
        return connections;
    }

    void bind(const sockaddr* address, socklen_t addressSize) {
        if (::bind(m_socket, address, addressSize) == -1) {
            perror("Error: Could not bind to port\n");
            close(m_socket);
            exit(1);
        }
    }

    void listen(int backlog) {
        if (::listen(m_socket, backlog) == -1) {
            std::cerr << "Error: Could not listen on socket\n";
            close(m_socket);
            exit(1);
        }
    }
    
    ssize_t send(const void* buffer, size_t length, int flags, const sockaddr* destAddr, socklen_t destAddrLen) {
        ssize_t bytesSent = sendto(m_socket, buffer, length, flags, destAddr, destAddrLen);
        if (bytesSent == -1) {
            perror("Error: Could not send data\n");
            throw std::runtime_error("Send error");
        }
        return bytesSent;
    }

    ssize_t recv(void* buffer, size_t length, int flags, sockaddr* srcAddr, socklen_t* srcAddrLen) {
        ssize_t bytesReceived = recvfrom(m_socket, buffer, length, flags, srcAddr, srcAddrLen);
        if (bytesReceived == -1) {
            perror("Error: Could not receive data\n");
            throw std::runtime_error("Receive error");
        }
        return bytesReceived;
    }

    // Get IP address
    std::string getIP() {
        sockaddr_in address;
        socklen_t addressSize = sizeof(address);
        getsockname(m_socket, (sockaddr*)&address, &addressSize);
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, ip, INET_ADDRSTRLEN);
        return ip;
    }

    // Get port
    int getPort() {
        sockaddr_in address;
        socklen_t addressSize = sizeof(address);
        getsockname(m_socket, (sockaddr*)&address, &addressSize);
        return ntohs(address.sin_port);
    }
    
private:
    int m_socket;

    // Connections
    std::vector<int> connections;
};

