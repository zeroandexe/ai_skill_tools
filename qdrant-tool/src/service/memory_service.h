#pragma once

#include "../adapter/qdrant_client.h"
#include "../adapter/ollama_client.h"
#include "../utils/uuid.h"
#include "../utils/text_chunker.h"
#include <chrono>

namespace qdrant {

// 添加记忆请求
struct AddMemoryRequest {
    std::string collection;
    std::string content;
    std::optional<std::string> id;
    std::optional<std::string> type;
    std::vector<std::string> tags;
    std::optional<std::string> source;
};

// 搜索记忆请求
struct SearchRequest {
    std::string collection;
    std::string query;
    int limit = 10;
    float minScore = 0.0f;
    std::optional<std::string> filterType;
    std::vector<std::string> filterTags;
    std::optional<std::string> filterSource;
    bool withContent = true;
};

// 更新记忆请求
struct UpdateMemoryRequest {
    std::string collection;
    std::string id;
    std::optional<std::string> content;
    std::optional<std::string> type;
    std::vector<std::string> addTags;
    std::vector<std::string> removeTags;
};

// 导入文件请求
struct ImportRequest {
    std::string filePath;
    std::string collection;
    TextChunker::Options chunkOptions;
    std::optional<std::string> type;
    std::vector<std::string> tags;
    std::optional<std::string> source;
};

// 导入统计
struct ImportStats {
    int totalChunks = 0;
    int successCount = 0;
    int failedCount = 0;
    std::vector<std::string> errors;
};

// 格式化后的搜索结果
struct FormattedSearchResult {
    std::string id;
    std::string content;
    float score;
    std::string type;
    std::vector<std::string> tags;
    int64_t timestamp;
    std::string datetime;
};

// 记忆服务类
class MemoryService {
public:
    MemoryService(QdrantClient& qdrant, OllamaClient& ollama);
    ~MemoryService() = default;
    
    // 添加记忆
    Result<std::string> add(const AddMemoryRequest& req);
    
    // 搜索记忆
    Result<std::vector<FormattedSearchResult>> search(const SearchRequest& req);
    
    // 删除记忆
    Result<bool> remove(const std::string& collection, const std::string& id);
    
    // 根据内容删除记忆
    Result<bool> removeByContent(const std::string& collection, const std::string& content);
    
    // 更新记忆
    Result<bool> update(const UpdateMemoryRequest& req);
    
    // 导入文件
    Result<ImportStats> importFile(const ImportRequest& req);
    
    // 获取记忆详情
    Result<std::optional<FormattedSearchResult>> get(const std::string& collection, 
                                                      const std::string& id);
    
    // 统计记忆数量
    Result<int64_t> count(const std::string& collection,
                          const std::optional<std::string>& type = std::nullopt,
                          const std::vector<std::string>& tags = {});
    
private:
    QdrantClient& qdrant_;
    OllamaClient& ollama_;
    
    // 构建 payload
    json buildPayload(const std::string& content);
    
    // 从 payload 提取格式化结果
    FormattedSearchResult formatResult(const SearchResult& result);
    
    // 确保集合存在
    Result<bool> ensureCollection(const std::string& name);
};

} // namespace qdrant
