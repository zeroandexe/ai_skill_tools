#include "collection_service.h"

namespace qdrant {

CollectionService::CollectionService(QdrantClient& qdrant) : qdrant_(qdrant) {}

Result<bool> CollectionService::create(const std::string& name,
                                        int dimension,
                                        const std::string& distance) {
    return qdrant_.createCollection(name, dimension, distance);
}

Result<bool> CollectionService::remove(const std::string& name) {
    return qdrant_.deleteCollection(name);
}

Result<std::vector<std::string>> CollectionService::list() {
    return qdrant_.listCollections();
}

Result<CollectionInfo> CollectionService::info(const std::string& name) {
    auto result = qdrant_.getCollectionInfo(name);
    if (result.isError()) {
        return makeError<CollectionInfo>(result.error().code, result.error().message);
    }
    
    try {
        auto jsonInfo = result.value();
        CollectionInfo info;
        info.name = name;
        info.dimension = jsonInfo["config"]["params"]["vectors"]["size"].get<int>();
        info.pointsCount = jsonInfo["points_count"].get<int64_t>();
        info.distance = jsonInfo["config"]["params"]["vectors"]["distance"].get<std::string>();
        info.createdAt = 0;  // Qdrant API 可能不包含创建时间
        
        return makeSuccess(info);
    } catch (const std::exception& e) {
        return makeError<CollectionInfo>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse collection info: " + std::string(e.what()));
    }
}

Result<bool> CollectionService::exists(const std::string& name) {
    return qdrant_.collectionExists(name);
}

} // namespace qdrant
