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

        fileManager = new FileManager(directoryPath);
        std::cout << "File Manager created" << std::endl;
        std::cout << "Current Directory: " << serverPath << std::endl;
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

            // // Receive data from client
            // ssize_t bytesReceived = serverSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrSize);

            // std::cout << "Received from client: " << buffer << std::endl;

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

    Message receiveAndUnmarshallRequest(int bytesReceived) {
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
        // Here, respond to each command

        // If input is "subscribe", wait 10 seconds then send a message
        // Retrieve first token
        // char* token = strtok(buffer, " ");
        // if (strcmp(token, "subscribe") == 0) {
        //     // handleSubscribe();
        //     return;
        // }

        // // Send data to client
        // serverSocket->send(buffer, bytesReceived, 0, (sockaddr*)&clientAddr, clientAddrSize);

        // Receive data from client

        Message receivedRequest = receiveAndUnmarshallRequest(bytesReceived);

        std::map<std::string, std::string> attributes = receivedRequest.bodyAttributes.attributes;

        int clientPort = std::stoi(attributes["port"]);
        clientAddr.sin_port = htons(clientPort); // set client port

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
        sendReply(reply);
    }

    void handleRead(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string offset = attributes["offset"];
        std::string length = attributes["length"];
        std::map<std::string, std::string> reply = fileManager->readFile(pathName, std::stoi(offset), std::stoi(length));
        sendReply(reply);
    }

    void handleInsert(std::map<std::string, std::string> attributes) {
        std::string pathName = attributes["pathName"];
        std::string offset = attributes["offset"];
        std::string content = attributes["content"];
        std::map<std::string, std::string> reply = fileManager->editFile(pathName, std::stoi(offset), content);
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
        std::string ipAddress = attributes["ipAddress"];
        std::string port = attributes["port"];

        // Check validity of timestamp
        if (std::stoi(timestamp) < time(0)) {
            std::cerr << "Error: Invalid timestamp" << std::endl;
            return;
        }

        // Check validaity of file name
        if (pathName != "file1" && pathName != "file2") {
            std::cerr << "Error: Invalid path name" << std::endl;
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
        
        AttributeMap reply;
        reply["response"] = message;
        sendReply(reply);
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
