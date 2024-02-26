#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <netinet/in.h> // For htonl and ntohl

struct BodyAttributes {
    std::map<std::string, std::string> attributes;

    // Marshalling function for BodyAttributes
    std::vector<uint8_t> marshal() const {
        std::vector<uint8_t> data;
        for (const auto& pair : attributes) {
            // Marshal attribute key length
            uint32_t keyLength = htonl(pair.first.length());
            appendBytes(data, reinterpret_cast<const uint8_t*>(&keyLength), sizeof(uint32_t));
            // Marshal attribute key
            appendBytes(data, reinterpret_cast<const uint8_t*>(pair.first.c_str()), pair.first.length());
            // Marshal attribute value length
            uint32_t valueLength = htonl(pair.second.length());
            appendBytes(data, reinterpret_cast<const uint8_t*>(&valueLength), sizeof(uint32_t));
            // Marshal attribute value
            appendBytes(data, reinterpret_cast<const uint8_t*>(pair.second.c_str()), pair.second.length());
        }
        return data;
    }

    // Unmarshalling function for BodyAttributes
    void unmarshal(const std::vector<uint8_t>& data) {
        size_t index = 0;
        while (index < data.size()) {
            // Unmarshal attribute key length
            uint32_t keyLength;
            std::memcpy(&keyLength, &data[index], sizeof(uint32_t));
            keyLength = ntohl(keyLength);
            index += sizeof(uint32_t);
            // Unmarshal attribute key
            std::string key(reinterpret_cast<const char*>(&data[index]), keyLength);
            index += keyLength;
            // Unmarshal attribute value length
            uint32_t valueLength;
            std::memcpy(&valueLength, &data[index], sizeof(uint32_t));
            valueLength = ntohl(valueLength);
            index += sizeof(uint32_t);
            // Unmarshal attribute value
            std::string value(reinterpret_cast<const char*>(&data[index]), valueLength);
            index += valueLength;
            // Insert attribute into map
            attributes[key] = value;
        }
    }

private:
    // Helper function to append bytes to vector
    void appendBytes(std::vector<uint8_t>& data, const uint8_t* bytes, size_t length) const {
        data.insert(data.end(), bytes, bytes + length);
    }
};

struct Message {
    int messageType;
    int requestId;
    BodyAttributes bodyAttributes;

    // Marshalling function for Request
    std::vector<uint8_t> marshal() const {
        std::vector<uint8_t> data;
        // Marshal messageType
        uint32_t messageTypeNet = htonl(messageType);
        appendBytes(data, reinterpret_cast<const uint8_t*>(&messageTypeNet), sizeof(uint32_t));
        // Marshal requestId
        uint32_t requestIdNet = htonl(requestId);
        appendBytes(data, reinterpret_cast<const uint8_t*>(&requestIdNet), sizeof(uint32_t));
        // Marshal bodyAttributes
        std::vector<uint8_t> bodyData = bodyAttributes.marshal();
        appendBytes(data, bodyData.data(), bodyData.size());
        return data;
    }

    // Unmarshalling function for Request
    void unmarshal(const std::vector<uint8_t>& data) {
        // Unmarshal messageType
        std::memcpy(&messageType, data.data(), sizeof(uint32_t));
        messageType = ntohl(messageType);
        // Unmarshal requestId
        std::memcpy(&requestId, &data[sizeof(uint32_t)], sizeof(uint32_t));
        requestId = ntohl(requestId);
        // Unmarshal bodyAttributes
        std::vector<uint8_t> bodyData(data.begin() + 2 * sizeof(uint32_t), data.end());
        bodyAttributes.unmarshal(bodyData);
    }

    void setVariables(int messageType, int requestId, const std::map<std::string, std::string>& attributes) {
        this->messageType = messageType;
        this->requestId = requestId;
        this->bodyAttributes.attributes = attributes;
    }

private:
    // Helper function to append bytes to vector
    void appendBytes(std::vector<uint8_t>& data, const uint8_t* bytes, size_t length) const {
        data.insert(data.end(), bytes, bytes + length);
    }

    
};