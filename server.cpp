#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "./socket.cpp"
#include "./filemanager.cpp"
#include "./utils.cpp"
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>

namespace std {
    template <>
    struct hash<std::pair<std::string, std::string>> {
        size_t operator()(const std::pair<std::string, std::string>& p) const {
            size_t hash = 0;
            hash_combine(hash, std::hash<std::string>{}(p.first));
            hash_combine(hash, std::hash<std::string>{}(p.second));
            return hash;
        }

        // Combine hash values using XOR
        template <typename T>
        static void hash_combine(size_t& seed, const T& val) {
            seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };

    template<>
    struct hash<std::vector<uint8_t>> {
        size_t operator()(const std::vector<uint8_t>& vec) const {
            size_t hash = 0;
            for (uint8_t elem : vec) {
                hash ^= std::hash<uint8_t>{}(elem) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
}

// Constants
const int PACKET_LOSS_RATE = 20;
const float DELAY_MEAN = 0.1; // seconds
const float DELAY_VARIANCE = 10; // seconds

class Server {
public:
    Server(int port, int invocationType) {
        this->invocationType = invocationType;
        // Create a socket
        serverSocket = new Socket(AF_INET, SOCK_DGRAM, 0);
        
        // Bind the socket to an IP address and port
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(port); // Server port
        serverSocket->bind((sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // BUGFIX: We need this line to initialize the clientAddrSize variable, which is used in the recvfrom function
        clientAddrSize = sizeof(clientAddr);

        std::cout << "Server listening on port " << port << std::endl;
    }

    void initFileManager() {
        std::string serverPath = fs::current_path().string();       
        std::string directoryPath = serverPath + "/ServerDirectory";

        fileManager = new FileManager(directoryPath);
        std::cout << "File Manager created" << std::endl;
        fileManager->clearDirectory();
        fileManager->viewDirectory();
    }
    
    void run() {
        initFileManager();

        while (true) {
            // Reset buffer
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesReceived = serverSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrSize);

            // if bytesReceived > 0, handle request
            if (bytesReceived > 0) {
                handleRequest(bytesReceived);
            }
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
    FileManager* fileManager;
    int invocationType;
    // map to store requests (clientAddress, clientPort, receivedData) and their corresponding responses
    std::unordered_map<std::pair<std::string, std::string>, std::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>>> requestResponseMap;

    char buffer[1024];
    // Subscriptions -- Filename : [Pair(timestamp, IP Address:Port), ...]
    std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> subscriptions;

    // Server helper functions
    // Handle request

    Message receiveAndUnmarshallRequest(std::vector<uint8_t> receivedData) {
        // Create a Message object and unmarshal the received data
        Message receivedRequest;
        receivedRequest.unmarshal(receivedData);

        return receivedRequest;
    }

    void sendReply(std::map<std::string, std::string> body, int requestId, std::vector<uint8_t> receivedData) {
        Message reply;
        reply.setVariables(1, requestId, body);
        std::vector<uint8_t> marshalledData = reply.marshal();
        // record request and response in map
        std::pair<std::string, std::string> clientAddress = std::make_pair(inet_ntoa(clientAddr.sin_addr), std::to_string(ntohs(clientAddr.sin_port)));
        requestResponseMap[clientAddress][receivedData] = marshalledData;

        serverSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
    }

    std::map<std::string, std::string> prepErrorReply(std::string message) {
        std::map<std::string, std::string> body;
        body["response"] = message;
        body["responseCode"] = "400";
        return body;
    }

    sockaddr_in getClientAddrFromIPAndPort(std::string ipAddress, int port) {
        sockaddr_in clientAddr;
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
        clientAddr.sin_port = htons(port);
        return clientAddr;
    }

    void sendTo(std::map<std::string, std::string> body, sockaddr_in clientAddr) { // TODO: track reply ID
        Message reply;
        reply.setVariables(1, 1, body);
        std::vector<uint8_t> marshalledData = reply.marshal();

        serverSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
    }

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

        // Receive data from client
        std::vector<uint8_t> receivedData(buffer, buffer + bytesReceived);        
        Message receivedRequest = receiveAndUnmarshallRequest(receivedData);
        int requestID = receivedRequest.requestId;
        std::map<std::string, std::string> attributes = receivedRequest.bodyAttributes.attributes;

        // Print IP
        std::string clientIP = getIPAddress(clientAddr);
        std::cout << "Client IP: " << clientIP << std::endl;

        // Print port
        int clientPort = getPort(clientAddr);
        std::cout << "Client port: " << clientPort << std::endl;

        // if invocation type == 1, check if request has been received before. if it has been received before, retransmit reply
        if (invocationType == 1) {
            std::pair<std::string, std::string> clientAddress = std::make_pair(inet_ntoa(clientAddr.sin_addr), std::to_string(clientPort));
            if (requestResponseMap.find(clientAddress) != requestResponseMap.end()) {
                if (requestResponseMap[clientAddress].find(receivedData) != requestResponseMap[clientAddress].end()) {
                    std::cout << "Retransmitting response" << std::endl;
                    std::vector<uint8_t> pastResponse = requestResponseMap[clientAddress][receivedData];
                    serverSocket->send(pastResponse.data(), pastResponse.size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
                    return;
                }
            }
        }
        std::string operation = attributes["operation"];
        std::cout << "Operation: " << operation << std::endl;
        std::map<std::string, std::string> reply;
        if (operation == "subscribe") {
            reply = handleSubscribe(attributes);
        }
        else if (operation == "create") {
            reply = handleCreate(attributes);
        }
        else if (operation == "delete") {
            reply = handleDelete(attributes);
        }
        else if (operation == "read") {
            reply = handleRead(attributes);
        }
        else if (operation == "insert") {
            reply = handleInsert(attributes);
        }
        else if (operation == "duplicate") {
            reply = handleDuplicate(attributes);
        } 
        else if (operation == "createdir") {
            reply = handleCreateDir(attributes);
        }
        else if (operation == "deletedir") {
            reply = handleDeleteDir(attributes);
        } else if (operation == "view") {
            reply = handleView(attributes);
        } else if (operation == "lastModified") {
            reply = handleLastModified(attributes);
        } else if (operation == "fileExists") {
            reply = handleFileExists(attributes);
        }
        else {
            handleEcho(receivedRequest, requestID, receivedData);
            return;
        }
        sendReply(reply, requestID, receivedData);
        return;
    }

    void handleEcho(Message receivedRequest, int requestID, std::vector<uint8_t> receivedData) {
        // Send data to client
        sendReply(receivedRequest.bodyAttributes.attributes, requestID, receivedData);
    }

    std::map<std::string, std::string> handleCreate(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->createFile(pathName);
        return reply;
    }

    std::map<std::string, std::string> handleDelete(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->deleteFile(pathName);
        publishUpdates();
        return reply;
    }

    std::map<std::string, std::string> handleRead(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string offset = attributes["offset"];
        std::string length = attributes["length"];
        std::map<std::string, std::string> reply = fileManager->readFile(pathName, offset, length);
        return reply;
    }

    std::map<std::string, std::string> handleInsert(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string offset = attributes["offset"];
        std::string content = attributes["content"];
        std::map<std::string, std::string> reply = fileManager->editFile(pathName, offset, content);
        publishUpdates();
        return reply;
    }

    std::map<std::string, std::string> handleDuplicate(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string newPathName = attributes["newPathName"];
        std::map<std::string, std::string> reply = fileManager->duplicateFile(pathName, newPathName);
        return reply;
    }

    std::map<std::string, std::string> handleCreateDir(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->createDirectory(pathName);
        return reply;
    }

    std::map<std::string, std::string> handleDeleteDir(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->deleteDirectory(pathName);
        return reply;
    }

    std::map<std::string, std::string> handleView(std::map<std::string, std::string> attributes) {
        std::map<std::string, std::string> reply = fileManager->viewDirectory();
        return reply;
    }

    std::map<std::string, std::string> handleSubscribe(std::map<std::string, std::string> attributes) {
        // Simulate waiting for data
        std::cout << "Subscribing..." << std::endl;

        // Retrieve attributes
        std::string pathName = attributes["pathName"];
        std::string timestamp = attributes["timestamp"];

        // Check validity of timestamp
        if (std::stoi(timestamp) < time(0)) {
            std::cerr << "Error: Invalid timestamp" << std::endl;
            std::map<std::string, std::string> reply = prepErrorReply("Invalid timestamp");
            return reply;
        }

        // Check validaity of file name
        if (!fileManager->fileExists(pathName)) {
            std::cerr << "Error: Invalid path name" << std::endl;
            std::map<std::string, std::string> reply = prepErrorReply("Invalid path name");
            return reply;
        }

        // Get client IP address and port
        std::string ipAddress = getIPAddress(clientAddr);
        int port = getPort(clientAddr);

        // Check if file is already subscribed by the client
        for (auto subscription : subscriptions[pathName]) {
            if (subscription.second == ipAddress + ":" + std::to_string(port)) {
                std::cerr << "Error: File already subscribed" << std::endl;
                std::map<std::string, std::string> reply = prepErrorReply("File already subscribed");
                return reply;
            }
        }

        // Save the subscription
        subscriptions[pathName].push_back(std::make_pair(std::stoi(timestamp), ipAddress + ":" + std::to_string(port)));
        
        // Send message to client
        const char* message = "You are now subscribed!"; // Simulate updated data
        
        AttributeMap reply;
        reply["response"] = message;
        reply["responseCode"] = "200";
        std::cout << "Sent to client: " << message << std::endl;

        std::thread unsubscribeThread([this, pathName, timestamp, ipAddress, port] {
            // Copy ip address and port
            std::string ipAddressCopy = ipAddress;
            int portCopy = port;
            
            std::cout << "Sleeping for " << std::stoi(timestamp) - time(0) << " seconds..." << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(std::stoi(timestamp) - time(0)));
            std::cout << "Unsubscribing..." << std::endl;
            // Send message to client
            const char* message = "Subscription terminated!"; // Simulate updated data
            AttributeMap reply;
            reply["response"] = message;
            reply["responseCode"] = "200";

            // Send to
            sendTo(reply, getClientAddrFromIPAndPort(ipAddressCopy, portCopy));

            std::cout << "Sent to client: " << message << std::endl;
            // Remove subscription
            subscriptions[pathName].erase(std::remove(subscriptions[pathName].begin(), subscriptions[pathName].end(), std::make_pair(std::stoi(timestamp), ipAddress + ":" + std::to_string(port))), subscriptions[pathName].end());
        });

        unsubscribeThread.detach();


        return reply;
    }

    std::map<std::string, std::string> handleLastModified(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::time_t lastModified = fileManager->getLastModifiedTime(pathName);
        std::map<std::string, std::string> reply = {{"lastModified", std::to_string(lastModified)}};
        return reply;
    }

    std::map<std::string, std::string> handleFileExists(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        // set reply with a response code 200 if file exists, 400 if it does not
        std::map<std::string, std::string> reply;
        if (fileManager->fileExists(pathName)) {
            reply["responseCode"] = "200";
        } else {
            reply["responseCode"] = "400";
            reply["response"] = "File does not exist";
        }
        return reply;
    }

    void publishUpdates() {
        // Iterate through subscriptions
        // If the current time is greater than the timestamp, 
        // send a message to the client to terminate the subscription
        
        for (auto& subscription : subscriptions) {
            std::string pathName = subscription.first;
            for (auto& client : subscription.second) {
                
                // Send message to client
                std::string message = "Received new content: \n" + fileManager->getFileContents(pathName);
                AttributeMap reply;
                reply["response"] = &message[0];
                reply["responseCode"] = "200";

                // Split IP address and port
                std::string ipAddress = client.second.substr(0, client.second.find(":"));
                int port = std::stoi(client.second.substr(client.second.find(":") + 1));

                // Send to
                sendTo(reply, getClientAddrFromIPAndPort(ipAddress, port));
                std::cout << "Sent to client: " << message << std::endl;
            }
        }
    }

    // Helpers

    std::string getIPAddress(sockaddr_in clientAddr) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
        return std::string(ip);
    }

    int getPort(sockaddr_in clientAddr) {
        return ntohs(clientAddr.sin_port);
    }
    
};

int main(int argc, char* argv[]) {
    
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);    
    // Run server on a separate thread
    int invocationType = atoi(argv[2]);

    Server server(port, invocationType);
    server.run();

    return 0;
}
