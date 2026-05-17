#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <set>

using namespace std;
namespace fs = std::filesystem;

void singleFileScan(const string& path);
void recursiveFileScan(const std::string& path, const std::string& keyword);
void searchInFile(const std::string& path, const std::string& keyword);


void singleFileScan(const string& path) {
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
					searchInFile(entry.path().string(), keyword);
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
		cout << "Usage: " << argv[0] << " <Target Directory> <Word to Search>";
		return 0;
	}

	string targetPath = argv[1];
	string searchWord = argv[2];

	if (!fs::exists(targetPath)) {
		cout << "Path does not exist: " << targetPath << "\n";
		return 1;
	}

	if (!fs::is_directory(targetPath)) {
		cout << "Path is not a directory: " << targetPath << "\n";
		return 1;
	}

	cout << "Scanning in: " << targetPath << "\n";
	cout << "Searching for: " << searchWord << "\n"; 

	recursiveFileScan(targetPath, searchWord);

	return 0;
}