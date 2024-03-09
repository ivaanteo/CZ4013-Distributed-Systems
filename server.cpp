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
    // std::unordered_map<std::pair<std::string, std::string>, 
    // std::unordered_map<std::pair<std::string, std::string>, std::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>>> requestResponseMap;

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

    void sendReply(std::map<std::string, std::string> body) { // TODO: track reply ID
        Message reply;
        reply.setVariables(1, 1, body);
        std::vector<uint8_t> marshalledData = reply.marshal();
        // record request and response in map
        // std::pair<std::string, std::string> clientAddress = std::make_pair(inet_ntoa(clientAddr.sin_addr), std::to_string(ntohs(clientAddr.sin_port)));
        // requestResponseMap[clientAddress][marshalledData] = marshalledData;
        
        serverSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
    }

    void sendErrorReply(std::string message) {
        std::map<std::string, std::string> body;
        body["response"] = message;
        body["responseCode"] = "400";
        sendReply(body);
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
        std::map<std::string, std::string> attributes = receivedRequest.bodyAttributes.attributes;

        // Print IP
        std::string clientIP = getIPAddress(clientAddr);
        std::cout << "Client IP: " << clientIP << std::endl;

        // Print port
        int clientPort = getPort(clientAddr);
        std::cout << "Client port: " << clientPort << std::endl;

        // if invocation type == 1, check if request has been received before. if it has been received before, retransmit reply
        // if (invocationType == 1) {
        //     std::pair<std::string, std::string> clientAddress = std::make_pair(inet_ntoa(clientAddr.sin_addr), std::to_string(clientPort));
        //     if (requestResponseMap.find(clientAddress) != requestResponseMap.end()) {
        //         if (requestResponseMap[clientAddress].find(receivedData) != requestResponseMap[clientAddress].end()) {
        //             std::vector<uint8_t> pastResponse = requestResponseMap[clientAddress][receivedData];
        //             serverSocket->send(pastResponse.data(), pastResponse.size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
        //             return;
        //         }
        //     }
        // }
        std::string operation = attributes["operation"];
        std::cout << "Operation: " << operation << std::endl;
        if (operation == "subscribe") {
            handleSubscribe(attributes);
        }
        else if (operation == "create") {
            handleCreate(attributes);
        }
        else if (operation == "delete") {
            handleDelete(attributes);
        }
        else if (operation == "read") {
            handleRead(attributes);
        }
        else if (operation == "insert") {
            handleInsert(attributes);
        }
        else if (operation == "duplicate") {
            handleDuplicate(attributes);
        } 
        else if (operation == "createdir") {
            handleCreateDir(attributes);
        }
        else if (operation == "deletedir") {
            handleDeleteDir(attributes);
        } else if (operation == "view") {
            handleView(attributes);
        }
        else {
            handleEcho(receivedRequest);
        }
    }

    void handleEcho(Message receivedRequest) {
        // Send data to client
        sendReply(receivedRequest.bodyAttributes.attributes);
    }

    void handleCreate(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->createFile(pathName);
        sendReply(reply);
    }

    void handleDelete(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->deleteFile(pathName);
        publishUpdates();
        sendReply(reply);
    }

    void handleRead(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string offset = attributes["offset"];
        std::string length = attributes["length"];
        std::map<std::string, std::string> reply = fileManager->readFile(pathName, offset, length);
        sendReply(reply);
    }

    void handleInsert(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string offset = attributes["offset"];
        std::string content = attributes["content"];
        std::map<std::string, std::string> reply = fileManager->editFile(pathName, offset, content);
        publishUpdates();
        sendReply(reply);
    }

    void handleDuplicate(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string newPathName = attributes["newPathName"];
        std::map<std::string, std::string> reply = fileManager->duplicateFile(pathName, newPathName);
        sendReply(reply);
    }

    void handleCreateDir(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->createDirectory(pathName);
        sendReply(reply);
    }

    void handleDeleteDir(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::map<std::string, std::string> reply = fileManager->deleteDirectory(pathName);
        sendReply(reply);
    }

    void handleView(std::map<std::string, std::string> attributes) {
        std::map<std::string, std::string> reply = fileManager->viewDirectory();
        sendReply(reply);
    }

    void handleSubscribe(std::map<std::string, std::string> attributes) {
        // Simulate waiting for data
        std::cout << "Subscribing..." << std::endl;

        // Retrieve attributes
        std::string pathName = attributes["pathName"];
        std::string timestamp = attributes["timestamp"];

        // Check validity of timestamp
        if (std::stoi(timestamp) < time(0)) {
            std::cerr << "Error: Invalid timestamp" << std::endl;
            sendErrorReply("Invalid timestamp");
            return;
        }

        // Check validaity of file name
        if (!fileManager->fileExists(pathName)) {
            std::cerr << "Error: Invalid path name" << std::endl;
            sendErrorReply("Invalid path name");
            return;
        }

        // Get client IP address and port
        std::string ipAddress = getIPAddress(clientAddr);
        int port = getPort(clientAddr);

        // Check if file is already subscribed by the client
        for (auto subscription : subscriptions[pathName]) {
            if (subscription.second == ipAddress + ":" + std::to_string(port)) {
                std::cerr << "Error: File already subscribed" << std::endl;
                sendErrorReply("File already subscribed");
                return;
            }
        }

        // Save the subscription
        subscriptions[pathName].push_back(std::make_pair(std::stoi(timestamp), ipAddress + ":" + std::to_string(port)));
        
        // Send message to client
        const char* message = "You are now subscribed!"; // Simulate updated data
        
        AttributeMap reply;
        reply["response"] = message;
        reply["responseCode"] = "200";
        sendReply(reply);
        std::cout << "Sent to client: " << message << std::endl;


        // Send unsubscription message to client on timestamp
        std::thread unsubscribeThread([this, pathName, timestamp, ipAddress, port] {
            // Copy ip address and port
            std::string ipAddressCopy = ipAddress;
            int portCopy = port;
            std::this_thread::sleep_for(std::chrono::seconds(std::stoi(timestamp) - time(0)));
            std::cout << "Unsubscribing..." << std::endl;
            // Send message to client
            const char* message = "Subscription terminated!"; // Simulate updated data
            AttributeMap reply;
            reply["response"] = message;
            reply["responseCode"] = "200";
            // Send timestamp of 1 second ago
            // reply["timestamp"] = std::to_string(time(0) - 1);

            // Send to
            sendTo(reply, getClientAddrFromIPAndPort(ipAddressCopy, portCopy));

            std::cout << "Sent to client: " << message << std::endl;
            // Remove subscription
            subscriptions[pathName].erase(std::remove(subscriptions[pathName].begin(), subscriptions[pathName].end(), std::make_pair(std::stoi(timestamp), ipAddress + ":" + std::to_string(port))), subscriptions[pathName].end());
        });

        unsubscribeThread.detach();
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
