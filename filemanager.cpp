#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <sys/stat.h>

namespace fs = std::filesystem;

class FileManager { // TODO: error handling
public:
    FileManager(const std::string& serverPath): serverPath(serverPath) {}

    std::string getDirectoryContentsAsString(const std::string& directoryPath, int depth = 0) {
        std::stringstream result;
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            std::stringstream ss;
            for (int i = 0; i < depth; ++i) {
                ss << "  "; // Adjust the indentation level according to the depth
            }
            ss << entry.path().filename().string();
            
            if (fs::is_directory(entry)) {
                result << ss.str() << " [Directory]\n";
                result << getDirectoryContentsAsString(entry.path().string(), depth + 1); // Recursive call for nested directories
            } else if (fs::is_regular_file(entry)) {
                result << ss.str() << " [File]\n";
            } else if (fs::is_symlink(entry)) {
                result << ss.str() << " [Symbolic Link]\n";
            }
        }
        return result.str();
    }

    std::map<std::string, std::string> viewDirectory() {
        std::map<std::string, std::string> response;
        std::cout << "Contents of Root Directory: " << serverPath << std::endl;
        if (!fs::exists(serverPath) || !fs::is_directory(serverPath)) {
            std::cerr << "Error: Server directory doesn't exist or is not a directory." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Server directory doesn't exist or is not a directory.";
            return response;
        }
        
        response["responseCode"] = "200";
        response["response"] = "Contents: \n" + getDirectoryContentsAsString(serverPath);
        return response;
    } 

    std::map<std::string, std::string> createDirectory(const std::string& pathName) {
        std::map<std::string, std::string> response;
        std::string newPath = serverPath + "/" + pathName;
        std::string tempPath = serverPath;

        if (fs::exists(newPath)) {
            std::cerr << "Error: Directory already exists." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Directory already exists.";
            return response;
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

        response["responseCode"] = "200";
        response["response"] = "Directory created.";
        return response;
    }

    void clearDirectory() {
        for (const auto& entry : fs::directory_iterator(serverPath)) {
            fs::remove_all(entry.path());
        }
        std::cout << "ServerDirectory cleared." << std::endl;
    }

    std::map<std::string, std::string> deleteDirectory(const std::string& pathName) {
        std::map<std::string, std::string> response;
        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: Directory doesn't exist." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Directory doesn't exist.";
            return response;
        }

        fs::remove_all(newPath);
        std::cout << "Directory " << pathName << " deleted." << std::endl;
        response["responseCode"] = "200";
        response["response"] = "Directory deleted.";
        return response;
    }
    
    std::map<std::string, std::string> createFile(const std::string& pathName) {
        std::map<std::string, std::string> response;
        std::string newPath = serverPath + "/" + pathName;
        if (fs::exists(newPath)) {
            std::cerr << "Error: File already exists." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "File already exists.";
            return response;
        }
        std::ofstream file(newPath);
        file.close();
        std::cout << "File " << pathName << " created." << std::endl;
        response["responseCode"] = "200";
        response["response"] = "File created.";
        return response;
    }

    std::map<std::string, std::string> deleteFile(const std::string& pathName) {
        std::map<std::string, std::string> response;

        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "File doesn't exist.";
            return response;
        }

        fs::remove(newPath);
        std::cout << "File " << pathName << " deleted." << std::endl;
        response["responseCode"] = "200";
        response["response"] = "File deleted.";
        return response;
    }
    
    // Read file. If length == -1, read until end of file
    std::map<std::string, std::string> readFile(const std::string& pathName, std::string offsetStr, std::string lengthStr) {
        std::map<std::string, std::string> response;

        // if offset or length is not a number, return error
        if (!std::all_of(offsetStr.begin(), offsetStr.end(), ::isdigit) || !std::all_of(lengthStr.begin(), lengthStr.end(), ::isdigit)) {
            std::cerr << "Error: Offset or length is not a number." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Offset or length is not a number.";
            return response;
        }

        // convert offset and length to int
        int offset = std::stoi(offsetStr);
        int length = std::stoi(lengthStr);

        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "File doesn't exist.";
            return response;
        }

        std::ifstream file(newPath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Could not open file.";
            return response;
        }

        // if offset exceeds length of file
        file.seekg(0, std::ios::end);
        int fileLength = file.tellg();
        if (offset > fileLength) {
            std::cerr << "Error: Offset exceeds length of file." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Offset exceeds length of file.";
            return response;
        }

        // if offset + length exceeds length of file, read until end of file
        if (offset + length > fileLength or length == -1) {
            length = fileLength - offset;
        }

        file.seekg(offset);
        char* buffer = new char[length];
        file.read(buffer, length);
        std::cout << "File " << pathName << " content: " << buffer << std::endl;
        file.close();

        // Add last modified time to response
        struct stat st;
        char* path = (char*)newPath.c_str();
        if (stat(path, &st) == -1) {
            std::cerr << "Error: Could not get last modified time." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Could not get last modified time.";
            return response;
        }
        response["lastModified"] = std::to_string(st.st_mtime);
        response["responseCode"] = "200";
        std::string buffer_str(buffer);
        response["response"] = buffer_str;
        return response;
    }

    std::map<std::string, std::string> editFile(const std::string& pathName, std::string offsetStr, const std::string& content) {
        std::map<std::string, std::string> response;
        // if offset or length is not a number, return error
        if (!std::all_of(offsetStr.begin(), offsetStr.end(), ::isdigit)) {
            std::cerr << "Error: Offset is not a number." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Offset is not a number.";
            return response;
        }

        // convert offset and length to int
        int offset = std::stoi(offsetStr);

        std::string newPath = serverPath + "/" + pathName;
        if (!fs::exists(newPath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "File doesn't exist.";
            return response;
        }

        std::fstream file(newPath, std::ios::in | std::ios::out);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Could not open file.";
            return response;
        }

        // if offset exceeds length of file
        file.seekg(0, std::ios::end);
        int length = file.tellg();
        if (offset > length) {
            std::cerr << "Error: Offset exceeds length of file." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "Offset exceeds length of file.";
            return response;
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

        response["responseCode"] = "200";
        response["response"] = "File edited.";
        return response;
    }

    std::map<std::string, std::string> duplicateFile(const std::string& oldPath, const std::string& newPath) {
        std::map<std::string, std::string> response;
        std::string oldFilePath = serverPath + "/" + oldPath;
        std::string newFilePath = serverPath + "/" + newPath;
        if (!fs::exists(oldFilePath)) {
            std::cerr << "Error: File doesn't exist." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "File doesn't exist.";
            return response;
        }

        // if new file already exists, return error
        if (fs::exists(newFilePath)) {
            std::cerr << "Error: New file already exists." << std::endl;
            response["responseCode"] = "400";
            response["response"] = "New file already exists.";
            return response;
        }

        fs::copy_file(oldFilePath, newFilePath);
        std::cout << "File " << oldPath << " duplicated to " << newPath << std::endl;

        response["responseCode"] = "200";
        response["response"] = "File duplicated.";
        return response;
    }

    // File exists
    bool fileExists(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        return fs::exists(newPath);
    }

    // Retrieve file contents as a string
    std::string getFileContents(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        std::ifstream file(newPath);
        std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
        return content;
    }

    std::time_t getLastModifiedTime(const std::string& pathName) {
        std::string newPath = serverPath + "/" + pathName;
        struct stat st;
        char* path = (char*)newPath.c_str();
        if (stat(path, &st) == -1) {
            std::cerr << "Error: Could not get last modified time." << std::endl;
            return -1;
        }
        return st.st_mtime;
    }

    private:
        std::string serverPath;
};
