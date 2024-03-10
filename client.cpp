#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "./socket.cpp"
#include <string>
#include "./utils.cpp"
class Client {
public:
    Client(int serverPort, int clientPort) {
        this->serverPort = serverPort;
        this->clientPort = clientPort;

        // Create a socket
        clientSocket = new Socket(AF_INET, SOCK_DGRAM, 0);

        // Create server address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(serverPort); // Server port

        // Set client address and port
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = INADDR_ANY; // Allow OS to assign the client IP
        clientAddr.sin_port = htons(clientPort); // Set client port

        clientSocket->bind((sockaddr*)&clientAddr, sizeof(clientAddr));
        std::cout << "Client running on port " << clientPort << std::endl;
    }

    void run() {
        
        while (true) {
            // Get user input
            std::string msg;
            getUserInput("Enter an operation. For more information, enter 'help': ", msg);
            handleUserInput(&msg[0]);
        }
    }

    ~Client() {
        delete clientSocket;
    }

private:
    Socket* clientSocket;
    sockaddr_in clientAddr;
    sockaddr_in serverAddr;
    socklen_t serverAddrSize;
    int clientPort;
    int serverPort;
    Message request;
    int requestId = 0;
    
    char buffer[1024];

    // Client helper functions

    void getUserInput(std::string message, std::string& input) {
        std::cout << message << std::endl;
        std::getline(std::cin, input);
    }
    
    void listenForUpdates(Message response) {
        int bytesReceived;
        while (response.bodyAttributes.attributes["responseCode"] == "200") {
            bytesReceived = receiveResponse();
            response = unmarshalResponse(bytesReceived);
            
            // If subscription terminated, break
            if (response.bodyAttributes.attributes["response"] == "Subscription terminated!") {
                std::cout << "Unsubscribing...\n";
                break;
            }
        }
    }

    void handleSubscribe() {

        std::string pathName;
        getUserInput("Enter the path name of the file you would like to subscribe to: ", pathName);

        std::string duration;
        getUserInput("Enter the duration you would like to subscribe for (in seconds): ", duration);
        std::string timestamp = std::to_string(time(0) + std::stoi(duration));

        // Send subscribe message to server
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "subscribe";
        requestBody["pathName"] = pathName;
        requestBody["timestamp"] = timestamp;
        sendRequest(requestBody);
        // Repeatedly receive updates from server
        ssize_t bytesReceived = receiveResponse();
        Message response = unmarshalResponse(bytesReceived);

        if (response.bodyAttributes.attributes["responseCode"] == "200") {
            // If successful, listen until timestamp
            listenForUpdates(response);
        }
    }

    void handleCreateFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to create: ", pathName);
        // exit if user enters exit
        if (pathName == "return") {
            std::cout << "Returning...\n";
            return;
        }
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "create";
        requestBody["pathName"] = pathName;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void handleDeleteFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to delete: ", pathName);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "delete";
        requestBody["pathName"] = pathName;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void handleReadFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to read: ", pathName);
        std::string offset;
        getUserInput("Enter the offset from which you would like to read: ", offset);
        std::string length;
        getUserInput("Enter the length which you would like to read: ", length);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "read";
        requestBody["pathName"] = pathName;
        requestBody["offset"] = offset;
        requestBody["length"] = length;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void handleInsertFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to insert into: ", pathName);
        std::string offset;
        getUserInput("Enter the offset at which you would like to insert: ", offset);
        std::string content;
        getUserInput("Enter the content you would like to insert: ", content);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "insert";
        requestBody["pathName"] = pathName;
        requestBody["offset"] = offset;
        requestBody["content"] = content;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void handleDuplicateFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to duplicate: ", pathName);
        std::string newPathName;
        getUserInput("Enter the path name of the new file: ", newPathName);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "duplicate";
        requestBody["pathName"] = pathName;
        requestBody["newPathName"] = newPathName;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void handleCreateDir() {
        std::string pathName;
        getUserInput("Enter the path name of the directory you would like to create: ", pathName);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "createdir";
        requestBody["pathName"] = pathName;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void handleDeleteDir() {
        std::string pathName;
        getUserInput("Enter the path name of the directory you would like to delete: ", pathName);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "deletedir";
        requestBody["pathName"] = pathName;
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    void sendRequest(std::map<std::string, std::string> body) {
        body["port"] = std::to_string(clientPort);
        Message request;
        request.setVariables(0, requestId, body);
        requestId++;
        std::vector<uint8_t> marshalledData = request.marshal();
        clientSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    void handleView() {
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "view";
        sendRequest(requestBody);
        ssize_t bytesReceived = receiveResponse();
        Message _ = unmarshalResponse(bytesReceived);
    }

    ssize_t receiveResponse() {
        ssize_t bytesReceived = clientSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, &serverAddrSize);
        if (bytesReceived == -1) {
            perror("Error: Could not receive data from server\n");
            return -1;
        }
        return bytesReceived;
    }

    Message unmarshalResponse(ssize_t bytesReceived) {
        // Unmarshal response
        Message response;
        response.unmarshal(std::vector<uint8_t>(buffer, buffer + bytesReceived));
        
        std::cout << "responseId: " << response.requestId << std::endl;


        // if successful, print result
        if (response.bodyAttributes.attributes["responseCode"] == "200") {
            std::cout << "Success! " << response.bodyAttributes.attributes["response"]  << std::endl;
            return response;
        }
        else {
            std::cerr << "Error: " << response.bodyAttributes.attributes["response"]  << std::endl;
            return response;
        }
    }

    void handleUserInput(char* input) {
        if (strcmp(input, "exit") == 0){
            std::cout << "Exiting...\n";
            exit(0);
        }
        else if (strcmp(input, "help") == 0){
            std::cout << "===========================================\n" 
                    << "Commands:\n"
                    << "view - View structure of main directory and files\n"
                    << "createdir - Create a new directory\n"
                    << "deletedir - Delete a directory\n"
                    << "create - Create empty file\n"
                    << "read - Read content from the file from offset\n"
                    << "insert - Insert content at a specific position in the file\n"
                    << "delete - Delete content from the file\n"
                    << "duplicate - Duplicate the file with a new filename\n"
                    << "help - Display this message\n"
                    << "exit - Exit the program\n"
                    << "subscribe - Subscribe to updates for a file\n"
                    << "===========================================\n"
                    << "\n";


            return;
        }
        else if (strcmp(input, "subscribe") == 0){
            handleSubscribe();
            return;
        }
        else if (strcmp(input, "create") == 0) {
            handleCreateFile();
        }
        else if (strcmp(input, "delete") == 0) {
            handleDeleteFile();
        } 
        else if (strcmp(input, "read") == 0) {
            handleReadFile();
        }
        else if (strcmp(input, "insert") == 0) {
            handleInsertFile();
        } 
        else if (strcmp(input, "duplicate") == 0) {
            handleDuplicateFile();
        }
        else if (strcmp(input, "createdir") == 0) {
            handleCreateDir();
        } 
        else if (strcmp(input, "deletedir") == 0) {
            handleDeleteDir();
        } 
        else if (strcmp(input, "view") == 0) {
            handleView();
        }
        else std::cout << "Invalid command. Type 'help' for a list of commands\n";   
    }
    
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <serverPort>" << " <clientPort>\n";
        exit(1);
    }

    int serverPort = std::stoi(argv[1]);
    int clientPort = std::stoi(argv[2]);
    
    Client client(serverPort, clientPort);

    client.run();

    return 0;
}


