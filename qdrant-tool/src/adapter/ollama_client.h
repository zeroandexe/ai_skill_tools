#pragma once

#include "../infrastructure/http_client.h"
#include "../infrastructure/result.h"
#include <vector>

namespace qdrant {

// Ollama 客户端类
class OllamaClient {
public:
    OllamaClient(const std::string& baseUrl, const std::string& model, int dimension = 1024);
    ~OllamaClient() = default;
    
    // 禁止拷贝，允许移动
    OllamaClient(const OllamaClient&) = delete;
    OllamaClient& operator=(const OllamaClient&) = delete;
    OllamaClient(OllamaClient&&) noexcept = default;
    OllamaClient& operator=(OllamaClient&&) noexcept = default;
    
    // 健康检查
    Result<bool> health();
    
    // 单文本向量化
    Result<std::vector<float>> embed(const std::string& text);
    
    // 批量向量化
    Result<std::vector<std::vector<float>>> embedBatch(const std::vector<std::string>& texts);
    
    // 获取模型信息
    Result<json> getModelInfo();
    
    // 获取ters
    const std::string& getModel() const { return model_; }
    int getDimension() const { return dimension_; }
    
private:
    HttpClient http_;
    std::string model_;
    int dimension_;
    
    // 构建嵌入请求
    json buildEmbedRequest(const std::vector<std::string>& texts) const;
};

} // namespace qdrant
