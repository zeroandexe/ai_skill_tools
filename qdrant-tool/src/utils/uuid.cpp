#include "uuid.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstring>

namespace qdrant {

std::mt19937& UUID::getGenerator() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

std::string UUID::generate() {
    std::uniform_int_distribution<int> dis(0, 255);
    auto& gen = getGenerator();
    
    // 生成 16 字节 (128 位) 随机数据
    uint8_t data[16];
    for (int i = 0; i < 16; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    
    // 设置版本 (v4) - 第 7 字节的高 4 位设为 0100
    data[6] = (data[6] & 0x0F) | 0x40;
    
    // 设置变体 - 第 9 字节的高 2 位设为 10
    data[8] = (data[8] & 0x3F) | 0x80;
    
    // 格式化为字符串
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            ss << '-';
        }
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    
    return ss.str();
}

std::string UUID::generateCompact() {
    std::string uuid = generate();
    std::string compact;
    compact.reserve(32);
    
    for (char c : uuid) {
        if (c != '-') {
            compact.push_back(c);
        }
    }
    
    return compact;
}

bool UUID::isValid(const std::string& uuid) {
    if (uuid.length() != 36) {
        return false;
    }
    
    // 检查横线位置
    if (uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-') {
        return false;
    }
    
    // 检查十六进制字符
    for (size_t i = 0; i < uuid.length(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            continue;
        }
        char c = uuid[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    
    return true;
}

} // namespace qdrant
