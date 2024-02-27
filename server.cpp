#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "./socket.cpp"
#include "./filemanager.cpp"
#include "./utils.cpp"
#include <map>
#include <string>

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

    void initFileManager() {
        std::string serverPath = fs::current_path().string();       
        std::string directoryPath = serverPath + "/ServerDirectory";

        FileManager fileManager(directoryPath);
        std::cout << "File Manager created" << std::endl;
        std::cout << "Current Directory: " << serverPath << std::endl;
        fileManager.clearDirectory();
        fileManager.viewDirectory();
    }
    
    void run() {
        initFileManager();

        while (true) {
            // Reset buffer
            memset(buffer, 0, sizeof(buffer));
            handleRequest();

            // // Receive data from client
            // ssize_t bytesReceived = serverSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrSize);

            // std::cout << "Received from client: " << buffer << std::endl;
            // clientAddr.sin_port = htons(8082); // set client port

            // // echo to the same client
            // ssize_t bytesSent = serverSocket->send(buffer, bytesReceived, 0, (sockaddr*)&clientAddr,  sizeof(clientAddr));

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
    FileManager* fileManager;

    char buffer[1024];
    // Subscriptions -- Filename : [Pair(timestamp, IP Address:Port), ...]
    // std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> subscriptions;

    // Server helper functions
    // Handle request

    Message receiveAndUnmarshallRequest() {
        ssize_t bytesReceived = serverSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrSize);
        
        // Unmarshal request
        std::vector<uint8_t> receivedData(buffer, buffer + bytesReceived);

        // Create a Message object and unmarshal the received data
        Message receivedRequest;
        receivedRequest.unmarshal(receivedData);

        return receivedRequest;
    }

    void sendReply(std::map<std::string, std::string> body) { // TODO: track reply ID
        Message reply;
        reply.setVariables(1, 1, body);
        std::vector<uint8_t> marshalledData = reply.marshal();
        std::cout << "Sending reply..." << std::endl;
        clientAddr.sin_port = htons(8082); // set client port
        serverSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
    }

    // void testMarshalling() {
    //     Message receivedRequest = receiveAndUnmarshallRequest();

    //     // Accessing fields of unmarshalled request
    //     std::cout << "Received MessageType: " << receivedRequest.messageType << std::endl;
    //     std::cout << "Received RequestId: " << receivedRequest.requestId << std::endl;
    //     std::cout << "Received BodyAttributes:" << std::endl;
    //     for (const auto& pair : receivedRequest.bodyAttributes.attributes) {
    //         std::cout << pair.first << ": " << pair.second << std::endl;
    //     }

    //     sendReply(receivedRequest.bodyAttributes.attributes);
    // }

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

    void handleRequest() {
        // Receive data from client
        Message receivedRequest = receiveAndUnmarshallRequest();
        std::map<std::string, std::string> attributes = receivedRequest.bodyAttributes.attributes;

        std::cout << "Received MessageType: " << receivedRequest.messageType << std::endl;
        std::cout << "Received RequestId: " << receivedRequest.requestId << std::endl;
        std::cout << "Received BodyAttributes:" << std::endl;
        for (const auto& pair : receivedRequest.bodyAttributes.attributes) {
            std::cout << pair.first << ": " << pair.second << std::endl;
        }

        std::string operation = attributes["operation"];
        std::cout << "Operation: " << operation << std::endl;
        // if (operation == "subscribe") {
        //     handleSubscribe(attributes);
        // }
        if (operation == "create") {
            handleCreate(attributes);
        }
    }

    void handleCreate(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string response = fileManager->createFile(pathName);
        std::map<std::string, std::string> reply;
        reply["response"] = response;
        sendReply(reply);
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
