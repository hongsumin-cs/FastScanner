#include <iostream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	// 인자 개수 부족
	if (argc < 3) {
		cout << "Usage: " << argv[0] << " <Target Directory> <Word to Search>";
		return 0;
	}

	string targetPath = argv[1];
	string searchWord = argv[2];

	// 목표 디렉토리 미존재
	if (!fs::exists(targetPath)) {
		cout << "Path does not exist: " << targetPath << "\n";
		return 1;
	}

	// 폴더가 아님
	if (!fs::is_directory(targetPath)) {
		cout << "Path is not a directory: " << targetPath << "\n";
		return 1;
	}

	cout << "Scanning in: " << targetPath << "\n";
	cout << "Searching for: " << searchWord << "\n";

	return 0;
}