#pragma once

#include "../infrastructure/http_client.h"
#include "../infrastructure/result.h"
#include <vector>
#include <optional>

namespace qdrant {

// 向量点结构
struct Point {
    std::string id;
    std::vector<float> vector;
    json payload;
};

// 搜索结果结构
struct SearchResult {
    std::string id;
    float score;
    json payload;
};

// 过滤条件结构
struct Filter {
    std::optional<std::string> type;
    std::vector<std::string> tags;
    std::optional<std::string> source;
    std::optional<std::pair<int64_t, int64_t>> timestampRange;  // [start, end]
    
    bool isEmpty() const {
        return !type.has_value() && tags.empty() && !source.has_value() && !timestampRange.has_value();
    }
};

// Qdrant 客户端类
class QdrantClient {
public:
    explicit QdrantClient(const std::string& baseUrl);
    ~QdrantClient() = default;
    
    // 禁止拷贝，允许移动
    QdrantClient(const QdrantClient&) = delete;
    QdrantClient& operator=(const QdrantClient&) = delete;
    QdrantClient(QdrantClient&&) noexcept = default;
    QdrantClient& operator=(QdrantClient&&) noexcept = default;
    
    // 健康检查
    Result<bool> health();
    
    // 集合操作
    Result<bool> createCollection(const std::string& name, 
                                   int dimension,
                                   const std::string& distance = "Cosine");
    Result<bool> deleteCollection(const std::string& name);
    Result<std::vector<std::string>> listCollections();
    Result<json> getCollectionInfo(const std::string& name);
    Result<bool> collectionExists(const std::string& name);
    
    // 向量操作
    Result<bool> upsert(const std::string& collection, 
                        const std::vector<Point>& points);
    Result<bool> deletePoints(const std::string& collection, 
                              const std::vector<std::string>& ids);
    Result<std::vector<SearchResult>> search(const std::string& collection,
                                              const std::vector<float>& vector,
                                              int limit = 10,
                                              float minScore = 0.0f,
                                              const Filter& filter = {});
    Result<std::optional<Point>> getPoint(const std::string& collection,
                                          const std::string& id);
    
    // 统计
    Result<int64_t> countPoints(const std::string& collection, 
                                 const Filter& filter = {});
    
private:
    HttpClient http_;
    
    // 构建过滤条件的 JSON
    json buildFilterJson(const Filter& filter) const;
    
    // 处理 HTTP 错误
    Error handleHttpError(const HttpResponse& response, ErrorCode defaultCode) const;
};

} // namespace qdrant
