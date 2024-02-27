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
    Client(int port) {
        // Create a socket
        clientSocket = new Socket(AF_INET, SOCK_DGRAM, 0);

        // Create server address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(port); // Server port

        // Set client address and port
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = INADDR_ANY; // Allow OS to assign the client IP
        clientAddr.sin_port = htons(8081); // Set client port

        clientSocket->bind((sockaddr*)&clientAddr, sizeof(clientAddr));
        std::cout << "Client bound to port "<< std::endl;
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
    
    char buffer[1024];

    // Client helper functions

    void getUserInput(std::string message, std::string& input) {
        std::cout << message << std::endl;
        std::getline(std::cin, input);
    }

    void handleSubscribe() {

        std::string pathName;
        getUserInput("Enter the path name of the file you would like to subscribe to: ", pathName);

        std::string duration;
        getUserInput("Enter the duration you would like to subscribe for (in seconds): ", duration);
        std::string timestamp = std::to_string(time(0) + std::stoi(duration));

        std::string ipAddress = clientSocket->getIP();
        std::string port = std::to_string(clientSocket->getPort());

        // Send subscribe message to server
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "subscribe";
        requestBody["pathName"] = pathName;
        requestBody["timestamp"] = timestamp;
        requestBody["ipAddress"] = ipAddress;
        requestBody["port"] = port;
        sendRequest(requestBody);
        // Repeatedly receive updates from server

        receiveResponse();


        // std::string request = "subscribe " + fileName + " " + timestamp + " " + ipAddress + " " + port;
        // std::map<std::string, std::string> requestBody;
        // requestBody["msg"] = request;
        // sendRequest(requestBody);


        // // Receive response on whether subscription was successful
        // clientSocket->recv(buffer, sizeof(buffer), 0);

        // memset(buffer, 0, sizeof(buffer)); // Temporarily clear buffer

        // If successful, wait for updates


        // Unsubscribe after duration
        
    }


    void handleCreateFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to create: ", pathName);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "create";
        requestBody["pathName"] = pathName;
        sendRequest(requestBody);
    }

    void handleDeleteFile() {
        std::string pathName;
        getUserInput("Enter the path name of the file you would like to delete: ", pathName);
        std::map<std::string, std::string> requestBody;
        requestBody["operation"] = "delete";
        requestBody["pathName"] = pathName;
        sendRequest(requestBody);
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
    }

    void sendRequest(std::map<std::string, std::string> body) { // TODO: track increasing request/ reply ID
        Message request;
        request.setVariables(0, 1, body);
        std::vector<uint8_t> marshalledData = request.marshal();
        clientSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        std::cout << "Request sent..." << std::endl;
        receiveResponse();
    }

    void receiveResponse() {
        ssize_t bytesReceived = clientSocket->recv(buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, &serverAddrSize);
        if (bytesReceived == -1) {
            perror("Error: Could not receive data from server\n");
            return;
        }

        // Unmarshal response
        Message response;
        response.unmarshal(std::vector<uint8_t>(buffer, buffer + bytesReceived));

        // if successful, print result
        if (response.bodyAttributes.attributes["responseCode"] == "200") {
            std::cout << "Success! " << response.bodyAttributes.attributes["response"]  << std::endl;
            return;
        }
        else {
            std::cerr << "Error: " << response.bodyAttributes.attributes["response"]  << std::endl;
            return;
        }
    }

    void handleUserInput(char* input) {
        if (strcmp(input, "exit") == 0){
            std::cout << "Exiting...\n";
            exit(0);
        }
        else if (strcmp(input, "help") == 0){
            std::cout << "Commands:\n"
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
                    << "subscribe - Subscribe to updates for a file\n";
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
        // if (strcmp(input, "view", 4) == 0) {
        //     handleView();
        // }
        // if (strcmp(input, "createdir") == 0) {
        //     handleCreateDir();
        // }
        // if (strcmp(input, "deletedir") == 0) {
        //     handleDeleteDir();
        // }

        // if (strcmp(input, "insert") == 0) {
        //     handleInsertFile();
        // }

        // if (strcmp(input, "duplicate") == 0) {
        //     handleDuplicateFile();
        // }
        else std::cout << "Invalid command. Type 'help' for a list of commands\n";   

        // TODO: Add more features here
        
    }
    
};

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        exit(1);
    }

    int port = std::stoi(argv[1]);
    
    Client client(port);

    client.run();

    return 0;
}


