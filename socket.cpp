#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <vector>

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

    int accept(sockaddr* address, socklen_t* addressSize) {
        int connectingSocket = ::accept(m_socket, address, addressSize);
        if (connectingSocket == -1) {
            std::cerr << "Error: Could not accept client connection\n";
            close(m_socket);
            exit(1);
        }
        // Add to list of connections
        connections.push_back(connectingSocket);
        return connectingSocket;
    }

    void connect(const sockaddr* address, socklen_t addressSize) {
        int connectingSocket = ::connect(m_socket, address, addressSize);
        if (connectingSocket == -1) {
            std::cerr << "Error: Could not connect to server\n";
            close(m_socket);
            exit(1);
        }
        // Add to list of connections
        connections.push_back(connectingSocket);
    }

    ssize_t send(const void* buffer, size_t length, int flags) {
        ssize_t bytesSent = ::send(m_socket, buffer, length, flags);
        if (bytesSent == -1) {
            std::cerr << "Error: Could not send data\n";
            close(m_socket);
            exit(1);
        }
        return bytesSent;
    }

    ssize_t recv(void* buffer, size_t length, int flags) {
        ssize_t bytesReceived = ::recv(m_socket, buffer, length, flags);
        if (bytesReceived == -1) {
            std::cerr << "Error: Could not receive data\n";
            close(m_socket);
            exit(1);
        }
        return bytesReceived;
    }
    
private:
    int m_socket;

    // Connections
    std::vector<int> connections;
};

