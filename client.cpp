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
    Client() {
        // Create a socket
        clientSocket = new Socket(AF_INET, SOCK_DGRAM, 0);

        // Create server address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
        serverAddr.sin_port = htons(8081); // Server port

        // Set client address and port
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = INADDR_ANY; // Allow OS to assign the client IP
        clientAddr.sin_port = htons(8082); // Set client port

        clientSocket->bind((sockaddr*)&clientAddr, sizeof(clientAddr));
        std::cout << "Client bound to port "<< std::endl;
    }

    void run() {
        
        while (true) {
            // Get user input
            std::string msg;
            getUserInput("Enter a message: ", msg);
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

        std::string fileName;
        getUserInput("Enter the file you would like to subscribe to: ", fileName);

        std::string duration;
        getUserInput("Enter the duration you would like to subscribe for (in seconds): ", duration);

        std::string timestamp = std::to_string(time(0) + std::stoi(duration));

        std::string ipAddress = clientSocket->getIP();
        std::string port = std::to_string(clientSocket->getPort());

        // // Send subscribe message to server
        std::string request = "subscribe " + fileName + " " + timestamp + " " + ipAddress + " " + port;
        std::map<std::string, std::string> requestBody;
        requestBody["msg"] = request;
        sendRequest(requestBody);


        // // Receive response on whether subscription was successful
        // clientSocket->recv(buffer, sizeof(buffer), 0);

        // memset(buffer, 0, sizeof(buffer)); // Temporarily clear buffer

        // If successful, wait for updates


        // Unsubscribe after duration
        
    }

    void sendRequest(std::map<std::string, std::string> body) { // TODO: track increasing request/ reply ID
        Message request;
        request.setVariables(0, 1, body);
        std::vector<uint8_t> marshalledData = request.marshal();
        clientSocket->send(marshalledData.data(), marshalledData.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

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

        std::cout << "Received from server: " << std::endl;
        std::cout << "Received MessageType: " << response.messageType << std::endl;
        std::cout << "Received RequestId: " << response.requestId << std::endl;
        std::cout << "Received BodyAttributes:" << std::endl;
        for (const auto& pair : response.bodyAttributes.attributes) {
            std::cout << pair.first << ": " << pair.second << std::endl;
        }
    }

    void handleUserInput(char* input) {
        if (strcmp(input, "exit") == 0){
            std::cout << "Exiting...\n";
            exit(0);
        }
        if (strcmp(input, "help") == 0){
            std::cout << "Commands:\n"
                    << "view - View structure of main directory and files\n"
                    << "createdir <pathname> - Create a new directory\n"
                    << "deletedir <pathname> - Delete a directory\n"
                    << "create <pathname> - Create empty file\n"
                    << "read <pathname> <offset (in bytes)> <number of bytes> - Read content from the file from offset\n"
                    << "insert <pathname> <offset (in bytes)> <content> - Insert content at a specific position in the file\n"
                    << "delete <pathname> - Delete content from the file\n"
                    << "duplicate <pathname> <new pathname> - Duplicate the file with a new filename\n"
                    << "help - Display this message\n"
                    << "exit - Exit the program\n"
                    << "subscribe - Subscribe to updates for a file\n";
            return;
        }
        if (strcmp(input, "subscribe") == 0){
            handleSubscribe();
            return;
        }


        // Echo input
        std::string msg = std::string(input) + "\n";
        std::map<std::string, std::string> requestBody;
        requestBody["msg"] = msg;
        sendRequest(requestBody);
        // TODO: Add more features here
        std::cout << "Invalid command. Type 'help' for a list of commands\n";   
    }
    
};

int main() {
    
    Client client;

    client.run();

    return 0;
}


