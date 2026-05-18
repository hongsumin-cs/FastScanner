#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <set>

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;

std::queue<std::string> pathQueue; 
std::mutex queueMutex;            
std::condition_variable cv; 
std::atomic<bool> isDirScanDone(false);

void worker(const std::string& keyword);
void singleFileScan(const std::string& path);
void recursiveFileScan(const std::string& path, const std::string& keyword);
void searchInFile(const std::string& path, const std::string& keyword);

void worker(const std::string& keyword) {
	while (true) {
		std::string currentPath;

		// critical section start
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
		// critical section end

		searchInFile(currentPath, keyword);
	}
}

void singleFileScan(const std::string& path) {
	for (const fs::directory_entry entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
		if (entry.is_directory()) {
			std::cout << "Directory: " << entry.path().string() << "\n";
		}

		else if (entry.is_regular_file()) {
			std::cout << "File: " << entry.path().string() << "\n";
		}
	}
}

void recursiveFileScan(const std::string& path, const std::string& keyword) {
	static const std::set<std::string> validExtensions = { ".txt", ".cpp", ".h", ".md", ".json", ".xml", ".csv" };

	try {
		for (const fs::directory_entry entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
			if (entry.is_directory()) {
				recursiveFileScan(entry.path().string(), keyword);
			}

			else if (entry.is_regular_file()) {
				std::string filename = entry.path().filename().string();

				if (filename.find(keyword) != std::string::npos) {
					std::cout << "File: " << entry.path().string() << "\n";
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

void searchInFile(const std::string& path, const std::string& keyword) {
	std::ifstream file(path);

	if (!file.is_open()) {
		return;
	}

	std::string line;
	int lineNumber = 1;

	while (std::getline(file, line)) {
		if (line.find(keyword) != std::string::npos) {
			std::cout << "Found in " << path << " (Line: " << lineNumber << ")\n";
		}
		lineNumber++;
	}
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " <Target Directory> <Word to Search>";
		return 0;
	}

	std::string targetPath = argv[1];
	std::string keyword = argv[2];

	if (!fs::exists(targetPath)) {
		std::cout << "Path does not exist: " << targetPath << "\n";
		return 1;
	}

	if (!fs::is_directory(targetPath)) {
		std::cout << "Path is not a directory: " << targetPath << "\n";
		return 1;
	}

	std::cout << "Scanning in: " << targetPath << "\n";
	std::cout << "Searching for: " << keyword << "\n";

	unsigned int numThreads = std::thread::hardware_concurrency();
	if (numThreads == 0) 
		numThreads = 4; // use 4 threads basically

	std::cout << "Using " << numThreads << " worker threads.\n";

	std::vector<std::thread> threadPool;
	for (unsigned int i = 0; i < numThreads; ++i) {
		threadPool.emplace_back(worker, keyword);
	}
	
	recursiveFileScan(targetPath, keyword);

	isDirScanDone = true;
	cv.notify_all();

	// join all threads
	for (auto& t : threadPool) {
		if (t.joinable()) {
			t.join();
		}
	}

	std::cout << "Scan complete!\n";
	return 0;
}