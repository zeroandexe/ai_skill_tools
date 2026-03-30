#include "config_manager.h"
#include "../../include/third_party/json.hpp"
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <linux/limits.h>
#include <iostream>

namespace qdrant {

using json = nlohmann::json;

// 默认配置常量（仅在此文件中使用，不暴露到外部）
namespace defaults {
    constexpr const char* QDRANT_URL = "http://localhost:6333";
    constexpr const char* OLLAMA_URL = "http://localhost:11434";
    constexpr const char* EMBEDDING_MODEL = "bge-m3";
    constexpr int EMBEDDING_DIMENSION = 1024;
    constexpr const char* DEFAULT_COLLECTION = "memory";
    constexpr int CONNECT_TIMEOUT = 5000;
    constexpr int REQUEST_TIMEOUT = 30000;
    constexpr bool ENABLE_LOG = false;
    constexpr const char* LOG_LEVEL = "info";
}

ConfigManager::ConfigManager() : loaded_(false) {
    // 初始化为空/无效值
    config_.embeddingDimension = 0;
    config_.connectTimeout = 0;
    config_.requestTimeout = 0;
    config_.enableLog = false;
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::load(int argc, char** argv) {
    if (loaded_) {
        return;  // 防止重复加载
    }
    
    loadedSources_.clear();
    
    // 1. 设置硬编码默认值（最低优先级）
    loadDefaults();
    loadedSources_.push_back("built-in defaults");
    
    // 2. 加载默认配置文件（如果存在）
    // 查找顺序：
    //   a) 安装目录/config/default.json
    std::string exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        std::string defaultConfigPath = exeDir + "/config/default.json";
        if (loadFromFile(defaultConfigPath)) {
            loadedSources_.push_back(defaultConfigPath);
        }
    }
    
    // 3. 加载系统级配置
    if (loadFromFile("/etc/qdrant-tool/config.json")) {
        loadedSources_.push_back("/etc/qdrant-tool/config.json");
    }
    
    // 4. 加载用户级配置
    const char* home = std::getenv("HOME");
    if (home) {
        std::string userConfigPath = std::string(home) + "/.config/qdrant-tool/config.json";
        if (loadFromFile(userConfigPath)) {
            loadedSources_.push_back(userConfigPath);
        }
    }
    
    // 5. 加载当前目录配置
    if (loadFromFile("./qdrant-tool.json")) {
        loadedSources_.push_back("./qdrant-tool.json");
    }
    
    // 6. 加载环境变量指定的配置
    if (auto envConfig = getEnv("QDRANT_TOOL_CONFIG")) {
        if (loadFromFile(*envConfig)) {
            loadedSources_.push_back(*envConfig + " (from QDRANT_TOOL_CONFIG)");
        }
    }
    
    // 7. 加载命令行指定的配置（如果提供了 -c/--config 参数）
    if (argc > 1 && argv != nullptr) {
        for (int i = 1; i < argc - 1; ++i) {
            std::string arg(argv[i]);
            if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
                std::string configPath(argv[i + 1]);
                if (loadFromFile(configPath)) {
                    loadedSources_.push_back(configPath + " (from command line)");
                }
                break;
            }
        }
    }
    
    // 8. 环境变量覆盖（最高优先级）
    loadFromEnv();
    
    loaded_ = true;
    
    // 验证配置
    if (!config_.isValid()) {
        std::cerr << "Warning: Configuration may be incomplete. Loaded from: " << std::endl;
        for (const auto& source : loadedSources_) {
            std::cerr << "  - " << source << std::endl;
        }
    }
}

void ConfigManager::loadDefaults() {
    config_.qdrantUrl = defaults::QDRANT_URL;
    config_.ollamaUrl = defaults::OLLAMA_URL;
    config_.embeddingModel = defaults::EMBEDDING_MODEL;
    config_.embeddingDimension = defaults::EMBEDDING_DIMENSION;
    config_.defaultCollection = defaults::DEFAULT_COLLECTION;
    config_.connectTimeout = defaults::CONNECT_TIMEOUT;
    config_.requestTimeout = defaults::REQUEST_TIMEOUT;
    config_.enableLog = defaults::ENABLE_LOG;
    config_.logLevel = defaults::LOG_LEVEL;
}

bool ConfigManager::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        json j;
        file >> j;
        
        // Qdrant 配置
        if (j.contains("qdrantUrl") && j["qdrantUrl"].is_string()) {
            config_.qdrantUrl = j["qdrantUrl"].get<std::string>();
        }
        
        // Ollama 配置
        if (j.contains("ollamaUrl") && j["ollamaUrl"].is_string()) {
            config_.ollamaUrl = j["ollamaUrl"].get<std::string>();
        }
        if (j.contains("embeddingModel") && j["embeddingModel"].is_string()) {
            config_.embeddingModel = j["embeddingModel"].get<std::string>();
        }
        if (j.contains("embeddingDimension") && j["embeddingDimension"].is_number()) {
            config_.embeddingDimension = j["embeddingDimension"].get<int>();
        }
        
        // 默认集合
        if (j.contains("defaultCollection") && j["defaultCollection"].is_string()) {
            config_.defaultCollection = j["defaultCollection"].get<std::string>();
        }
        
        // 超时配置
        if (j.contains("connectTimeout") && j["connectTimeout"].is_number()) {
            config_.connectTimeout = j["connectTimeout"].get<int>();
        }
        if (j.contains("requestTimeout") && j["requestTimeout"].is_number()) {
            config_.requestTimeout = j["requestTimeout"].get<int>();
        }
        
        // 日志配置
        if (j.contains("enableLog") && j["enableLog"].is_boolean()) {
            config_.enableLog = j["enableLog"].get<bool>();
        }
        if (j.contains("logLevel") && j["logLevel"].is_string()) {
            config_.logLevel = j["logLevel"].get<std::string>();
        }
        
        return true;
    } catch (const std::exception& e) {
        // 解析失败，静默返回 false
        return false;
    }
}

void ConfigManager::loadFromEnv() {
    bool envLoaded = false;
    
    // Qdrant 配置
    if (auto url = getEnv("QDRANT_URL")) {
        config_.qdrantUrl = *url;
        envLoaded = true;
    }
    
    // Ollama 配置
    if (auto url = getEnv("OLLAMA_URL")) {
        config_.ollamaUrl = *url;
        envLoaded = true;
    }
    if (auto model = getEnv("EMBEDDING_MODEL")) {
        config_.embeddingModel = *model;
        envLoaded = true;
    }
    if (auto dim = getEnv("EMBEDDING_DIMENSION")) {
        try {
            config_.embeddingDimension = std::stoi(*dim);
            envLoaded = true;
        } catch (...) {
            // 忽略无效的数值
        }
    }
    
    // 默认集合
    if (auto collection = getEnv("DEFAULT_COLLECTION")) {
        config_.defaultCollection = *collection;
        envLoaded = true;
    }
    
    // 超时配置
    if (auto timeout = getEnv("CONNECT_TIMEOUT")) {
        try {
            config_.connectTimeout = std::stoi(*timeout);
            envLoaded = true;
        } catch (...) {}
    }
    if (auto timeout = getEnv("REQUEST_TIMEOUT")) {
        try {
            config_.requestTimeout = std::stoi(*timeout);
            envLoaded = true;
        } catch (...) {}
    }
    
    // 日志配置
    if (auto log = getEnv("ENABLE_LOG")) {
        config_.enableLog = (*log == "true" || *log == "1");
        envLoaded = true;
    }
    if (auto level = getEnv("LOG_LEVEL")) {
        config_.logLevel = *level;
        envLoaded = true;
    }
    
    if (envLoaded) {
        loadedSources_.push_back("environment variables");
    }
}

bool ConfigManager::saveToFile(const std::string& path) const {
    try {
        json j;
        j["qdrantUrl"] = config_.qdrantUrl;
        j["ollamaUrl"] = config_.ollamaUrl;
        j["embeddingModel"] = config_.embeddingModel;
        j["embeddingDimension"] = config_.embeddingDimension;
        j["defaultCollection"] = config_.defaultCollection;
        j["connectTimeout"] = config_.connectTimeout;
        j["requestTimeout"] = config_.requestTimeout;
        j["enableLog"] = config_.enableLog;
        j["logLevel"] = config_.logLevel;
        
        // 确保目录存在
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dir = path.substr(0, lastSlash);
            std::string cmd = "mkdir -p '" + dir + "'";
            int ret = system(cmd.c_str());
            (void)ret;  // 忽略返回值
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

std::string ConfigManager::toString() const {
    json j;
    j["qdrantUrl"] = config_.qdrantUrl;
    j["ollamaUrl"] = config_.ollamaUrl;
    j["embeddingModel"] = config_.embeddingModel;
    j["embeddingDimension"] = config_.embeddingDimension;
    j["defaultCollection"] = config_.defaultCollection;
    j["connectTimeout"] = config_.connectTimeout;
    j["requestTimeout"] = config_.requestTimeout;
    j["enableLog"] = config_.enableLog;
    j["logLevel"] = config_.logLevel;
    j["_sources"] = loadedSources_;
    return j.dump(2);
}

std::optional<std::string> ConfigManager::getEnv(const std::string& name) const {
    const char* value = std::getenv(name.c_str());
    if (value) {
        return std::string(value);
    }
    return std::nullopt;
}

std::optional<std::string> ConfigManager::findConfigFile() const {
    // 1. 检查环境变量
    if (auto envConfig = getEnv("QDRANT_TOOL_CONFIG")) {
        if (access(envConfig->c_str(), R_OK) == 0) {
            return envConfig;
        }
    }
    
    // 2. 检查当前目录
    if (access("./qdrant-tool.json", R_OK) == 0) {
        return std::string("./qdrant-tool.json");
    }
    
    // 3. 检查用户配置
    const char* home = std::getenv("HOME");
    if (home) {
        std::string userConfig = std::string(home) + "/.config/qdrant-tool/config.json";
        if (access(userConfig.c_str(), R_OK) == 0) {
            return userConfig;
        }
    }
    
    // 4. 检查系统配置
    if (access("/etc/qdrant-tool/config.json", R_OK) == 0) {
        return std::string("/etc/qdrant-tool/config.json");
    }
    
    // 5. 检查安装目录
    std::string exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        std::string installConfig = exeDir + "/config/default.json";
        if (access(installConfig.c_str(), R_OK) == 0) {
            return installConfig;
        }
    }
    
    return std::nullopt;
}

std::string ConfigManager::getExecutableDir() const {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            return fullPath.substr(0, lastSlash);
        }
    }
    return "";
}

} // namespace qdrant
