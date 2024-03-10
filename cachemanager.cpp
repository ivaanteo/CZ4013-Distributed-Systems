#include <iostream>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <string>
#include <sys/stat.h>
#include <vector>

struct CacheRange {
    std::string data;
    int start;
    int end;
};

struct CacheEntry {
    std::time_t lastValidated; // So everytime we call, we update last validated
    std::time_t lastModified; // If last modified is not the same as the last modified of the file, we remove the entry
    // Ok so if any time the last modified doesn't match, we remove the entry
    // When last validated is not within the freshness interval, check if the last modified is the same as the file
    // If it is, then we can use the cache, if not, we remove the entry
    std::vector<CacheRange> ranges;
};

class CacheManager {
public:
    void insert(const std::string& key, const std::string& value, int start, int end, std::time_t lastModified) {
        std::cout << "Insert with key: " << key << " value: " << value << " start: " << start << " end: " << end << " lastModified: " << lastModified << std::endl;
        // If the key doesn't exist, we insert it
        if (cache.find(key) == cache.end()) {
            std::cout << "Inserting new key" << std::endl;
            cache[key].lastValidated = std::time(nullptr);
            cache[key].lastModified = lastModified;
            cache[key].ranges.push_back({value, start, end});
            return;
        } 
        // If the key exists, first check if the last modified is the same as the file
        // If it is, we update the cache
        // If it's not, we remove the entry and insert the new one
        if (cache[key].lastModified != lastModified) {
            std::cout << "Removing old key" << std::endl;
            cache[key].lastValidated = std::time(nullptr);
            cache[key].lastModified = lastModified;
            cache[key].ranges.clear();
            cache[key].ranges.push_back({value, start, end});
            return;
        } 

        // If the last modified is the same, we merge overlapping ranges
        // If the range is not overlapping, we insert the new range
        if (cache[key].ranges.empty()) {
            std::cout << "Inserting new range" << std::endl;
            cache[key].lastValidated = std::time(nullptr);
            cache[key].ranges.push_back({value, start, end});
            return;
        }
        if (start > cache[key].ranges.back().end) {
            std::cout << "Inserting at the back" << std::endl;
            cache[key].lastValidated = std::time(nullptr);
            cache[key].ranges.push_back({value, start, end});
            return;
        }
        if (end < cache[key].ranges.front().start) {
            std::cout << "Inserting at the front" << std::endl;
            cache[key].lastValidated = std::time(nullptr);
            cache[key].ranges.insert(cache[key].ranges.begin(), {value, start, end});
            return;
        }

        // Insert the new range
        cache[key].lastValidated = std::time(nullptr);
        cache[key].ranges.push_back({value, start, end});

        // Sort the ranges
        std::sort(cache[key].ranges.begin(), cache[key].ranges.end(), [](const CacheRange& a, const CacheRange& b) {
            return a.start < b.start;
        });

        std::cout << "Sorted ranges" << std::endl;
        
        // Merge overlapping ranges
        int i = 0;
        while (i < cache[key].ranges.size() - 1) {
            if (cache[key].ranges[i].end >= cache[key].ranges[i + 1].start) {
                
                std::cout << "Before merging" << std::endl;
                print();
                // Check if partially overlapping
                if (cache[key].ranges[i].end < cache[key].ranges[i + 1].end and cache[key].ranges[i].end >= cache[key].ranges[i + 1].start) {
                    cache[key].ranges[i].data += cache[key].ranges[i + 1].data.substr(cache[key].ranges[i].end - cache[key].ranges[i + 1].start, cache[key].ranges[i + 1].end - cache[key].ranges[i].end + 1);
                }
                cache[key].ranges.erase(cache[key].ranges.begin() + i + 1);
                
                cache[key].ranges[i].end = std::max(cache[key].ranges[i].end, cache[key].ranges[i + 1].end);
                // Merged ranges
                std::cout << "After merging" << std::endl;
                print();
                
            } else {
                i++;
            }
        }
    }

    // This function assumes that the key exists and data is fresh
    std::string get(const std::string& key, int start, int end) {
        if (cache.find(key) != cache.end()) {
            for (auto& range : cache[key].ranges) {
                if (range.start <= start && range.end >= end) {
                    return range.data.substr(start, end - start + 1);
                }
            }
        }
        return "";
    }

    void print() {
        for (auto& entry : cache) {
            std::cout << entry.first << " " << entry.second.lastValidated << " " << entry.second.lastModified << std::endl;
            for (auto& range : entry.second.ranges) {
                std::cout << range.start << " " << range.end << " : " << range.data << std::endl;
            }
        }
    }

    void setFreshnessInterval(int interval) {
        freshnessInterval = interval;
    }

    bool isRangeCached(const std::string& key, int start, int end) {
        if (cache.find(key) != cache.end()) {
            for (auto& range : cache[key].ranges) {
                if (range.start <= start && range.end >= end) {
                    return true;
                }
            }
        }
        return false;
    }

    bool isLastValidatedFresh(const std::string& key, int start, int end) {
        if (cache.find(key) != cache.end()) {
            return std::time(nullptr) - cache[key].lastValidated < freshnessInterval;
        }
        return false;
    }

    bool isLastModifiedFresh(const std::string& key, int start, int end, std::time_t lastModified) {
        if (cache.find(key) != cache.end()) {
            return cache[key].lastModified == lastModified;
        }
        return false;
    }

    void invalidate(const std::string& key) {
        if (cache.find(key) != cache.end()) {
            cache.erase(key);
        }
    }

private:
    std::unordered_map<std::string, CacheEntry> cache;
    int freshnessInterval = 60;
};
