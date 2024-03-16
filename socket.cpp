#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <random>
#include <thread>
#include <stdexcept>



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

    int getSocket() {
        return m_socket;
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
    
    ssize_t send(const void* buffer, size_t length, int flags, const sockaddr* destAddr, socklen_t destAddrLen) {
        ssize_t bytesSent = sendto(m_socket, buffer, length, flags, destAddr, destAddrLen);
                // Simulate a loss in packets
        if (shouldDrop()) {
            std::cout << "Dropping packet" << std::endl;
            return -1;
        }

        // Sleep for delay
        int delay = getDelay() * 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        if (bytesSent == -1) {
            perror("Error: Could not send data\n");
            throw std::runtime_error("Send error");
        }
        return bytesSent;
    }

    ssize_t recv(void* buffer, size_t length, int flags, sockaddr* srcAddr, socklen_t* srcAddrLen) {
        ssize_t bytesReceived = recvfrom(m_socket, buffer, length, flags, srcAddr, srcAddrLen);
        return bytesReceived;
    }
    
private:
    int m_socket;

    // Shitpuckery
    static const int PACKET_LOSS_RATE = 10;
    constexpr static const float DELAY_MEAN = 1.0f; // In seconds
    constexpr static const float DELAY_VARIANCE = 0.1f;

    // Simulating Network Loss
    bool shouldDrop(int percent = PACKET_LOSS_RATE) {
        return (rand() % 100) < percent;
    }

    // Choose delay from a binomial distribution
    float getDelay(float mean = DELAY_MEAN, float variance = DELAY_VARIANCE) {
        std::default_random_engine generator;
        std::normal_distribution<float> distribution(mean, variance);
        float delay = distribution(generator);
        return std::max(delay, 0.0f);
    }

    // Connections
    std::vector<int> connections;
};

