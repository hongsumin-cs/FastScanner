#include <iostream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#include "FastScanner.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <Target Directory> <Word to Search>\n";
        std::cout << "Options:\n";
        std::cout << "  -n   Search in file names only\n";
        std::cout << "  -c   Search in file contents only\n";
        return 0;
    }

    std::string targetPath = argv[1];
    std::string keyword = argv[2];

    bool searchName = true;
    bool searchContent = true;

    if (!fs::exists(targetPath) || !fs::is_directory(targetPath)) {
        std::cout << "Invalid Path: " << targetPath << "\n";
        return 1;
    }

    // Check 4th argument (Searching option)
    if (argc >= 4) {
        std::string option = argv[3];
        if (option == "-n") {
            searchContent = false; // Search by name
        }
        else if (option == "-c") {
            searchName = false;    // Search by content
        }
    }

    std::cout << "Scanning in: " << targetPath << "\n";
    std::cout << "Searching for: " << keyword << "\n";

    try {
        FastScanner scanner(keyword);

        scanner.setSearchMode(searchName, searchContent);

        // Print immediately when a result is found
        scanner.setOnResultFound([](const SearchResult& result) {
            std::string text;
            if (result.type == SearchResult::FileNameMatch)
                text = "File: " + result.path + "\n";
            else
                text = "Found in " + result.path + " (Line: " + std::to_string(result.lineNumber) + ")\n";
            std::cout << text;
            });

        scanner.startScan(targetPath); // Start Scan
        std::cout << scanner.getResults().size() << " matches in " << scanner.getScannedFileCount() << " files.\n";
    } catch (const std::invalid_argument& e) {
        // Handle invalid argument error
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    catch (const std::exception& e) {
        // Catch other errors
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Scan complete!\n";
    return 0;
}