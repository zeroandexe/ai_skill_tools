#include "file_reader.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace qdrant {

Result<std::string> FileReader::readAll(const std::string& path) {
    if (!exists(path)) {
        return makeError<std::string>(ErrorCode::FILE_READ_FAILED,
            "File not found: " + path);
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return makeError<std::string>(ErrorCode::FILE_READ_FAILED,
            "Cannot open file: " + path);
    }
    
    try {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return makeSuccess(buffer.str());
    } catch (const std::exception& e) {
        return makeError<std::string>(ErrorCode::FILE_READ_FAILED,
            "Failed to read file: " + std::string(e.what()));
    }
}

Result<std::vector<std::string>> FileReader::readLines(const std::string& path) {
    if (!exists(path)) {
        return makeError<std::vector<std::string>>(ErrorCode::FILE_READ_FAILED,
            "File not found: " + path);
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return makeError<std::vector<std::string>>(ErrorCode::FILE_READ_FAILED,
            "Cannot open file: " + path);
    }
    
    try {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        return makeSuccess(lines);
    } catch (const std::exception& e) {
        return makeError<std::vector<std::string>>(ErrorCode::FILE_READ_FAILED,
            "Failed to read file: " + std::string(e.what()));
    }
}

bool FileReader::exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

Result<size_t> FileReader::getSize(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return makeError<size_t>(ErrorCode::FILE_READ_FAILED,
            "Cannot stat file: " + path);
    }
    return makeSuccess(static_cast<size_t>(buffer.st_size));
}

} // namespace qdrant
