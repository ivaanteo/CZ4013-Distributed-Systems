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

    void createDirectory(const std::string& directoryName) {
        std::string newPath = serverPath + "/" + directoryName;
        std::string tempPath = serverPath;

        if (fs::exists(newPath)) {
            std::cerr << "Error: Directory already exists." << std::endl;
            return;
        }

        // split directoryName using '/' as delimiter
        std::string delimiter = "/";
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = directoryName.find(delimiter, pos_start)) != std::string::npos) {
            token = directoryName.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back(directoryName.substr(pos_start));
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

private:
    std::string serverPath;
};
