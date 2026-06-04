#pragma once
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class FastScanner {
private:
    std::queue<std::string> pathQueue;
    std::mutex queueMutex;
    std::mutex printMutex;
    std::condition_variable cv;
    std::atomic<bool> isDirScanDone;

    std::vector<std::thread> threadPool;
    std::string keyword;

    void worker();
    void recursiveFileScan(const std::string& path, std::vector<std::string>& batch);
    void searchInFile(const std::string& path);

public:
    FastScanner(const std::string& searchWord, unsigned int threadCount = 0);
    ~FastScanner();

    void startScan(const std::string& targetPath);
};