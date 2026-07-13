#pragma once
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <filesystem>

// Struct for search result
struct SearchResult {
    std::string path;
    int lineNumber;
    enum Type { FileNameMatch, ContentMatch } type;
};

// Struct for scanning
struct ScanTask {
    bool isDirectory;
    std::filesystem::path path;
};

class FastScanner {
private:
    std::queue<ScanTask> taskQueue; // Queue for pushing tasks
    std::mutex queueMutex;
    std::mutex printMutex;
    std::mutex resultMutex;
    std::vector<SearchResult> results; // Vector to save searched results

    std::condition_variable workerCv; // CV for workers
    std::condition_variable doneCv; // CV for main thread
    std::atomic<int> activeWorkers{ 0 }; // Number of workers active
    std::atomic<bool> stopPool = false; // Thread pool controller

    std::vector<std::thread> threadPool;
    std::string keyword;

    void worker();
    void directoryScan(const std::filesystem::path& path);
    void searchInFile(const std::filesystem::path& path);

    // Callback function pointer
    std::function<void(const SearchResult&)> onResultFound;

public:
    FastScanner(const std::string& searchWord, unsigned int threadCount = 0);
    ~FastScanner();

    void startScan(const std::filesystem::path& targetPath);

    std::vector<SearchResult> getResults();

    // Callback for onResultFound
    void setOnResultFound(std::function<void(const SearchResult&)> callback) {
        std::lock_guard<std::mutex> lock(resultMutex);
        onResultFound = std::move(callback);
    }
};