#include <iostream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


void singleFileScan(const string& path) {
	for (const fs::directory_entry entry : fs::directory_iterator(path)) {
		if (entry.is_directory()) {
			std::cout << "Directory: " << entry.path().string() << "\n";
		}

		else if (entry.is_regular_file()) {
			std::cout << "File: " << entry.path().string() << "\n";
		}
	}
}

void recursiveFileScan(const std::string& path) {
	try {
		for (const fs::directory_entry entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
			if (entry.is_directory()) {
				recursiveFileScan(entry.path().string());
			}

			else if (entry.is_regular_file()) {
				std::cout << "File: " << entry.path().string() << "\n";
			}
		}
	}

	catch (const fs::filesystem_error& e) {
		std::cerr << "Error: " << path << "\n";
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

	return 0;
}