#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>

namespace fs = std::filesystem;

class FileManager {
public:
    FileManager(const std::string& serverPath): serverPath(serverPath) {}

    void viewDirectory() {
        std::cout << "Contents of Root Directory: " << serverPath << std::endl;
        if (!fs::exists(serverPath) || !fs::is_directory(serverPath)) {
            std::cerr << "Error: Server directory doesn't exist or is not a directory." << std::endl;
            return;
        }
        
        int depth = 0;
        printDirectoryContents(serverPath, depth);
    }

    void printDirectoryContents(const std::string& directoryPath, int depth = 0) {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            std::stringstream ss;
            for (int i = 0; i < depth; ++i) {
                ss << "  "; // Adjust the indentation level according to the depth
            }
            ss << entry.path().filename().string();
            
            if (fs::is_directory(entry)) {
                std::cout << ss.str() << " [Directory]" << std::endl;
                printDirectoryContents(entry.path().string(), depth + 1); // Recursive call for nested directories
            } else if (fs::is_regular_file(entry)) {
                std::cout << ss.str() << " [File]" << std::endl;
            } else if (fs::is_symlink(entry)) {
                std::cout << ss.str() << " [Symbolic Link]" << std::endl;
            }
        }
    } 

    void createDirectory(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        std::string tempPath = serverPath;

        if (fs::exists(newPath)) {
            std::cerr << "Error: Directory already exists." << std::endl;
            return;
        }

        // split pathName using '/' as delimiter
        std::string delimiter = "/";
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = pathName.find(delimiter, pos_start)) != std::string::npos) {
            token = pathName.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back(pathName.substr(pos_start));
        for (std::string dir: res) {
            tempPath += "/" + dir;
            fs::create_directory(tempPath);
            std::cout << "Directory " << dir << " created." << std::endl;
        }
    }

    void clearDirectory() {
        for (const auto& entry : fs::directory_iterator(serverPath)) {
            fs::remove_all(entry.path());
        }
        std::cout << "ServerDirectory cleared." << std::endl;
    }

    void deleteDirectory(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: Directory doesn't exist." << std::endl;
            return;
        }

        fs::remove_all(newPath);
        std::cout << "Directory " << pathName << " deleted." << std::endl;
    }
    
    void createFile(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        std::ofstream file(newPath);
        file.close();
        std::cout << "File " << pathName << " created." << std::endl;
    }

    void deleteFile(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            return;
        }

        fs::remove(newPath);
        std::cout << "File " << pathName << " deleted." << std::endl;
    }

    void duplicateFile(const std::string& oldPath, const std::string& newPath) {
        std::string oldFilePath = serverPath + "/" + oldPath;
        std::string newFilePath = serverPath + "/" + newPath;
        if (!fs::exists(oldFilePath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            return;
        }

        fs::copy_file(oldFilePath, newFilePath);
        std::cout << "File " << oldPath << " duplicated to " << newPath << std::endl;
    }

    void editFile(const std::string& pathName, int offset, const std::string& content) {
        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            return;
        }

        std::fstream file(newPath, std::ios::in | std::ios::out);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file." << std::endl;
            return;
        }

        // if offset exceeds length of file
        file.seekg(0, std::ios::end);
        int length = file.tellg();
        if (offset > length) {
            std::cerr << "Error: Offset exceeds length of file." << std::endl;
            return;
        }

        file.seekp(offset);
        std::string remainingContent;
        remainingContent.resize(length - offset);
        file.seekg(offset);
        file.read(&remainingContent[0], remainingContent.size());

        // Move the file pointer to the offset and write the new content
        file.seekp(offset);
        file.write(content.c_str(), content.size());

        // Write back the existing content after the new content
        file.write(remainingContent.c_str(), remainingContent.size());
        file.close();
        std::cout << "File " << pathName << " edited." << std::endl;
    }

    void readFile(const std::string& pathName, int offset, int length) {
        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            return;
        }

        std::ifstream file(newPath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file." << std::endl;
            return;
        }

        // if offset exceeds length of file
        file.seekg(0, std::ios::end);
        int fileLength = file.tellg();
        if (offset > fileLength) {
            std::cerr << "Error: Offset exceeds length of file." << std::endl;
            return;
        }

        // if offset + length exceeds length of file, read until end of file
        if (offset + length > fileLength) {
            length = fileLength - offset;
        }

        file.seekg(offset);
        char* buffer = new char[length];
        file.read(buffer, length);
        std::cout << "File " << pathName << " content: " << buffer << std::endl;
        file.close();
    }

    private:
        std::string serverPath;
};
