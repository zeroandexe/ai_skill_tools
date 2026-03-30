#pragma once

#include "../infrastructure/result.h"
#include <string>
#include <vector>

namespace qdrant {

// 文件读取工具类
class FileReader {
public:
    // 读取整个文件内容为字符串
    static Result<std::string> readAll(const std::string& path);
    
    // 按行读取文件
    static Result<std::vector<std::string>> readLines(const std::string& path);
    
    // 检查文件是否存在且可读
    static bool exists(const std::string& path);
    
    // 获取文件大小
    static Result<size_t> getSize(const std::string& path);
};

} // namespace qdrant
