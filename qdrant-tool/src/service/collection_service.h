#pragma once

#include "../adapter/qdrant_client.h"

namespace qdrant {

// 集合信息
struct CollectionInfo {
    std::string name;
    int dimension;
    int64_t pointsCount;
    std::string distance;
    int64_t createdAt;
};

// 集合服务类
class CollectionService {
public:
    explicit CollectionService(QdrantClient& qdrant);
    ~CollectionService() = default;
    
    // 创建集合
    Result<bool> create(const std::string& name, 
                        int dimension = 1024,
                        const std::string& distance = "Cosine");
    
    // 删除集合
    Result<bool> remove(const std::string& name);
    
    // 列出所有集合
    Result<std::vector<std::string>> list();
    
    // 获取集合信息
    Result<CollectionInfo> info(const std::string& name);
    
    // 检查集合是否存在
    Result<bool> exists(const std::string& name);
    
private:
    QdrantClient& qdrant_;
};

} // namespace qdrant
