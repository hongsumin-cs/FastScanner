#include "FastScanner.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>
#include <algorithm>
#include <string_view>

// Cross OS API header
#ifdef _WIN32
	// Windows
	#include <windows.h>
#else
	// mac, Linux
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
#	include <unistd.h>
#endif

namespace fs = std::filesystem;

// Construct of FastScanner
// Create thread pool
FastScanner::FastScanner(const std::string& searchWord, unsigned int numThreads) : keyword(searchWord)
{
	if (numThreads == 0) {
		numThreads = std::thread::hardware_concurrency();

		if (numThreads == 0)
			numThreads = 4; // Use 4 threads basically
	}

	for (unsigned int i = 0; i < numThreads; ++i) {
		threadPool.emplace_back(&FastScanner::worker, this);
	}

	std::cout << "Using " << numThreads << " worker threads.\n";
}

// Destructor of FastScanner
FastScanner::~FastScanner() {
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		stopPool = true; // Stop thread pool
	}

	workerCv.notify_all();

	// Joint all threads
	// Left for safety
	for (auto& t : threadPool) {
		if (t.joinable()) {
			t.join();
		}
	}
}

// Push only root directory and wait
void FastScanner::startScan(const std::string& targetPath) {
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		taskQueue.push({ true, targetPath });
	}

	workerCv.notify_one();

	{
		std::unique_lock<std::mutex> lock(queueMutex);
		doneCv.wait(lock, [this] {
			return taskQueue.empty() && activeWorkers == 0; // Queue is empty & No worker is working
		});
	}
}


void FastScanner::worker() {
	while (true) {
		std::vector<ScanTask> batch; // Batch to bring works from task queue
		batch.reserve(64); // Prevent vector reallocation overhead

		// Critical section start
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			workerCv.wait(lock, [this] {
				return !taskQueue.empty() || stopPool;
			});

			if (stopPool && taskQueue.empty()) 
				return;

			// Bring works from queue to batch
			while (!taskQueue.empty() && batch.size() < 64) {
				batch.push_back(taskQueue.front());
				taskQueue.pop();
			}
			activeWorkers++; // Working thread +1
		}
		// Critical section end

		for (const auto& task : batch) {
			if (task.isDirectory) {
				directoryScan(task.path); // If directory, thread itself open inner directory
			}
			else {
				searchInFile(task.path); // If file, search text in it
			}
		}

		{
			std::lock_guard<std::mutex> lock(queueMutex);
			activeWorkers--; // Working thread -1

			if (taskQueue.empty() && activeWorkers == 0) {
				doneCv.notify_one(); // Awake main thread and notice work is done
			}
		}
	}
}


// Search file
// Not in use
/*
void FastScanner::singleFileScan(const std::string& path) {
	for (const fs::directory_entry entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
		if (entry.is_directory()) {
			std::cout << "Directory: " << entry.path().string() << "\n";
		}

		else if (entry.is_regular_file()) {
			std::cout << "File: " << entry.path().string() << "\n";
		}
	}
}
*/


// No batch as argument anymore
// Use batch space
// Scan inner directories and push into task queue with batch
// To solve race condition while scanning directory
void FastScanner::directoryScan(const std::string& path) {
	static const std::set<std::string> validExtensions = { ".txt", ".cpp", ".h", ".md", ".json", ".xml", ".csv" };

	std::vector<ScanTask> batch; // Batch to push tasks into task queue
	batch.reserve(64); // Prevent vector reallocation overhead

	try {
		for (const fs::directory_entry entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
			// Destructor check
			if (stopPool) 
				break;

			if (entry.is_directory()) {
				// Convert inner directory to task and push
				batch.push_back({ true, entry.path().string() });
			}

			else if (entry.is_regular_file()) {
				std::string filename = entry.path().filename().string();

				// Found filename match
				if (filename.find(keyword) != std::string::npos) {
					{
						std::lock_guard<std::mutex> lock(printMutex);
						std::cout << "File: " << entry.path().string() << "\n";
					}
				}

				std::string extension = entry.path().extension().string();
				if (validExtensions.find(extension) != validExtensions.end()) {
					// Convert file to task and push
					batch.push_back({ false, entry.path().string() });
				}
			}

			// When batch is full, push into main task queue
			if (batch.size() >= 64) {
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					for (const auto& t : batch)
						taskQueue.push(t);
				}

				workerCv.notify_all();
				batch.clear();
			}
		}

		// Push all leftover tasks in batch
		if (!batch.empty()) {
			std::lock_guard<std::mutex> lock(queueMutex);
			for (const auto& t : batch)
				taskQueue.push(t);
			workerCv.notify_all();
		}
	}

	catch (const fs::filesystem_error& e) {
		std::cerr << "Error: " << path << "\n";
	}
}

/* Standard fstream based search
void searchInFile(const std::string& path, const std::string& keyword) {
	std::ifstream file(path);

	if (!file.is_open()) {
		return;
	}

	std::string line;
	int lineNumber = 1;

	while (std::getline(file, line)) {
		if (line.find(keyword) != std::string::npos) {
			{
				std::lock_guard<std::mutex> lock(printMutex);
				std::cout << "Found in " << path << " (Line: " << lineNumber << ")\n";
			}
		}

		lineNumber++;
	}
}
*/


// Zero-copy based search function
void FastScanner::searchInFile(const std::string& path) {
	const char* mappedData = nullptr;
	size_t fileSize = 0;

	// Windows API
#ifdef _WIN32
	// Open file for read only
	HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Check file size
	LARGE_INTEGER size;
	if (GetFileSizeEx(hFile, &size)) {
		fileSize = size.QuadPart;
	}

	HANDLE hMap = NULL;
	if (fileSize > 0) {
		hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMap) {
			mappedData = (const char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		}
	}

	// mac/Linux API
#else
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return;

	struct stat st; // Struct for inode

	if (fstat(fd, &st) == 0) {
		fileSize = st.st_size;
		if (fileSize > 0) {
			mappedData = (const char*)mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
			if (mappedData == MAP_FAILED) {
				mappedData = nullptr;
			}
		}
	}
#endif


	// Keyword search & line tracking
	if (mappedData && fileSize > 0) {
		std::string_view fileView(mappedData, fileSize);

		size_t offset = 0;
		size_t foundPos;

		int lineNumber = 1;
		size_t lastCheckedLine = 0; // Save line number of last keyword

		// Search keyword in file
		while ((foundPos = fileView.find(keyword, offset)) != std::string_view::npos) {
			lineNumber += std::count(mappedData + lastCheckedLine, mappedData + foundPos, '\n');
			lastCheckedLine = foundPos;

			{
				std::lock_guard<std::mutex> lock(printMutex);
				std::cout << "Found in " << path << " (Line: " << lineNumber << ")\n";
			}

			offset = foundPos + keyword.length();
		}
	}


	// Unmapping
#ifdef _WIN32
	if (mappedData)
		UnmapViewOfFile(mappedData);
	if (hMap)
		CloseHandle(hMap);
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
#else
	if (mappedData)
		munmap((void*)mappedData, fileSize);
	if (fd >= 0)
		close(fd);
#endif
}