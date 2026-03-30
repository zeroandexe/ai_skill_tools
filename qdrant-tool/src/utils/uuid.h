#pragma once

#include <string>
#include <random>

namespace qdrant {

// UUID 生成器类
class UUID {
public:
    // 生成 v4 UUID (随机)
    static std::string generate();
    
    // 生成不带横线的 UUID
    static std::string generateCompact();
    
    // 验证 UUID 格式
    static bool isValid(const std::string& uuid);
    
private:
    // 随机数生成器
    static std::mt19937& getGenerator();
};

} // namespace qdrant
