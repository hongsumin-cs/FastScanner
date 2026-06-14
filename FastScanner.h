#pragma once
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Struct for scanning
struct ScanTask {
    bool isDirectory;
    std::string path;
};

class FastScanner {
private:
    std::queue<ScanTask> taskQueue; // Queue for pushing tasks
    std::mutex queueMutex;
    std::mutex printMutex;

    std::condition_variable workerCv; // cv for workers
    std::condition_variable doneCv; // cv for main thread
    std::atomic<int> activeWorkers{ 0 }; // Number of workers active
    std::atomic<bool> stopPool = false; // Thread pool controller

    std::vector<std::thread> threadPool;
    std::string keyword;

    void worker();
    void directoryScan(const std::string& path);
    void searchInFile(const std::string& path);

public:
    FastScanner(const std::string& searchWord, unsigned int threadCount = 0);
    ~FastScanner();

    void startScan(const std::string& targetPath);
};