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
FastScanner::FastScanner(const std::string& searchWord, unsigned int numThreads)
	: keyword(searchWord), isDirScanDone(false)
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
	isDirScanDone = true;
	cv.notify_all();

	// Joint all threads
	// Left for safety
	for (auto& t : threadPool) {
		if (t.joinable()) {
			t.join();
		}
	}
}

// Call recursive file scanning
void FastScanner::startScan(const std::string& targetPath) {
	recursiveFileScan(targetPath);

	isDirScanDone = true; // Notice directory scan is done
	cv.notify_all(); // Wake all workers

	// Wait until all threads' work done!!!
	for (auto& t : threadPool) {
		if (t.joinable()) {
			t.join();
		}
	}
}

void FastScanner::worker() {
	while (true) {
		std::string currentPath;

		// Critical section start
		{
			std::unique_lock<std::mutex> lock(queueMutex);

			while (pathQueue.empty() && !isDirScanDone) {
				cv.wait(lock);
			}

			if (isDirScanDone && pathQueue.empty()) {
				return;
			}

			currentPath = pathQueue.front();
			pathQueue.pop();
		}
		// Critical section end

		searchInFile(currentPath);
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

void FastScanner::recursiveFileScan(const std::string& path) {
	static const std::set<std::string> validExtensions = { ".txt", ".cpp", ".h", ".md", ".json", ".xml", ".csv" };

	try {
		for (const fs::directory_entry entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
			if (entry.is_directory()) {
				recursiveFileScan(entry.path().string());
			}

			else if (entry.is_regular_file()) {
				std::string filename = entry.path().filename().string();

				if (filename.find(keyword) != std::string::npos) {
					{
						std::lock_guard<std::mutex> lock(printMutex);
						std::cout << "File: " << entry.path().string() << "\n";
					}
				}

				std::string extension = entry.path().extension().string();

				if (validExtensions.find(extension) != validExtensions.end()) {
					{
						std::lock_guard<std::mutex> lock(queueMutex);
						pathQueue.push(entry.path().string());
					}

					cv.notify_one();
				}
			}
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
