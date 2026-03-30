#pragma once

#include <string>
#include <optional>
#include <vector>

namespace qdrant {

// 应用程序配置结构
// 不设置默认值，所有值从配置文件加载
struct Config {
    // Qdrant 配置
    std::string qdrantUrl;
    
    // Ollama 配置
    std::string ollamaUrl;
    std::string embeddingModel;
    int embeddingDimension;
    
    // 默认集合
    std::string defaultCollection;
    
    // 超时配置（毫秒）
    int connectTimeout;
    int requestTimeout;
    
    // 日志配置
    bool enableLog;
    std::string logLevel;
    
    // 检查配置是否已加载
    bool isValid() const {
        return !qdrantUrl.empty() && !ollamaUrl.empty() && embeddingDimension > 0;
    }
};

// 配置管理器类
class ConfigManager {
public:
    // 获取单例实例
    static ConfigManager& getInstance();
    
    // 加载配置（优先级：命令行 > 环境变量 > 用户配置 > 系统配置 > 默认配置）
    // 配置文件搜索路径：
    //   1. $QDRANT_TOOL_CONFIG (环境变量指定)
    //   2. ./qdrant-tool.json (当前目录)
    //   3. ~/.config/qdrant-tool/config.json (用户配置)
    //   4. /etc/qdrant-tool/config.json (系统配置)
    //   5. <安装目录>/config/default.json (默认配置)
    void load(int argc = 0, char** argv = nullptr);
    
    // 获取当前配置
    const Config& getConfig() const { return config_; }
    Config& getConfig() { return config_; }
    
    // 从环境变量加载（覆盖现有值）
    void loadFromEnv();
    
    // 从配置文件加载（覆盖现有值）
    bool loadFromFile(const std::string& path);
    
    // 保存到配置文件
    bool saveToFile(const std::string& path) const;
    
    // 获取配置加载来源
    const std::vector<std::string>& getLoadedSources() const { return loadedSources_; }
    
    // 打印当前配置（用于调试）
    std::string toString() const;
    
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    // 禁止拷贝
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    Config config_;
    std::vector<std::string> loadedSources_;
    bool loaded_;
    
    // 加载默认配置
    void loadDefaults();
    
    // 获取环境变量值
    std::optional<std::string> getEnv(const std::string& name) const;
    
    // 查找配置文件路径
    std::optional<std::string> findConfigFile() const;
    
    // 获取可执行文件所在目录
    std::string getExecutableDir() const;
};

} // namespace qdrant
