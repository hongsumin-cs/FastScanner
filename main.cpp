#include <iostream>
#include <filesystem>
#include "FastScanner.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <Target Directory> <Word to Search>\n";
        return 0;
    }

    std::string targetPath = argv[1];
    std::string keyword = argv[2];

    if (!fs::exists(targetPath) || !fs::is_directory(targetPath)) {
        std::cout << "Invalid Path: " << targetPath << "\n";
        return 1;
    }

    std::cout << "Scanning in: " << targetPath << "\n";
    std::cout << "Searching for: " << keyword << "\n";

    try {
        FastScanner scanner(keyword);

        // Print immediately when a result is found
        scanner.setOnResultFound([](const SearchResult& result) {
            if (result.type == SearchResult::FileNameMatch)
                std::cout << "File: " << result.path << "\n";
            else
                std::cout << "Found in " << result.path << " (Line: " << result.lineNumber << ")\n";
            });

        scanner.startScan(targetPath); // Start Scan
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