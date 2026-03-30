#include "memory_service.h"
#include "../utils/file_reader.h"
#include "../infrastructure/config_manager.h"
#include <iomanip>
#include <ctime>
#include <sstream>

namespace qdrant {

MemoryService::MemoryService(QdrantClient& qdrant, OllamaClient& ollama)
    : qdrant_(qdrant)
    , ollama_(ollama) {}

Result<std::string> MemoryService::add(const AddMemoryRequest& req) {
    // 1. 向量化内容
    auto embedResult = ollama_.embed(req.content);
    if (embedResult.isError()) {
        return makeError<std::string>(embedResult.error().code,
            "Failed to embed content: " + embedResult.error().message);
    }
    
    // 2. 确保集合存在
    auto ensureResult = ensureCollection(req.collection);
    if (ensureResult.isError()) {
        return makeError<std::string>(ensureResult.error().code,
            ensureResult.error().message);
    }
    
    // 3. 检查是否存在完全相同的 content
    // 使用向量搜索查找相似度接近 1.0 的记录
    auto searchResult = qdrant_.search(req.collection, embedResult.value(), 10, 0.99f);
    if (searchResult.isSuccess()) {
        for (const auto& sr : searchResult.value()) {
            // 检查 content 是否完全相同
            if (sr.payload.contains("content") && 
                sr.payload["content"].get<std::string>() == req.content) {
                // 找到完全相同的 content，更新 datetime
                Point point;
                point.id = sr.id;
                point.vector = embedResult.value();  // 使用当前的 embedding（相同文本的 embedding 相同）
                point.payload = buildPayload(req.content);
                
                auto upsertResult = qdrant_.upsert(req.collection, {point});
                if (upsertResult.isError()) {
                    return makeError<std::string>(upsertResult.error().code,
                        "Failed to update memory: " + upsertResult.error().message);
                }
                return makeSuccess(sr.id);  // 返回原有记录的 id
            }
        }
    }
    
    // 4. 不存在相同 content，插入新记录
    std::string id = req.id.value_or(UUID::generate());
    
    // 5. 构建 payload
    auto payload = buildPayload(req.content);
    
    // 6. 构建 point
    Point point;
    point.id = id;
    point.vector = embedResult.value();
    point.payload = payload;
    
    // 7. 插入到 Qdrant
    auto upsertResult = qdrant_.upsert(req.collection, {point});
    if (upsertResult.isError()) {
        return makeError<std::string>(upsertResult.error().code,
            "Failed to save memory: " + upsertResult.error().message);
    }
    
    return makeSuccess(id);
}

Result<std::vector<FormattedSearchResult>> MemoryService::search(const SearchRequest& req) {
    // 1. 向量化查询
    auto embedResult = ollama_.embed(req.query);
    if (embedResult.isError()) {
        return makeError<std::vector<FormattedSearchResult>>(embedResult.error().code,
            "Failed to embed query: " + embedResult.error().message);
    }
    
    // 2. 构建过滤条件
    Filter filter;
    if (req.filterType.has_value()) {
        filter.type = req.filterType;
    }
    if (!req.filterTags.empty()) {
        filter.tags = req.filterTags;
    }
    if (req.filterSource.has_value()) {
        filter.source = req.filterSource;
    }
    
    // 3. 执行搜索
    auto searchResult = qdrant_.search(req.collection, embedResult.value(), 
                                        req.limit, req.minScore, filter);
    if (searchResult.isError()) {
        return makeError<std::vector<FormattedSearchResult>>(searchResult.error().code,
            searchResult.error().message);
    }
    
    // 4. 格式化结果
    std::vector<FormattedSearchResult> results;
    for (const auto& sr : searchResult.value()) {
        results.push_back(formatResult(sr));
    }
    
    return makeSuccess(results);
}

Result<bool> MemoryService::remove(const std::string& collection, const std::string& id) {
    return qdrant_.deletePoints(collection, {id});
}

Result<bool> MemoryService::removeByContent(const std::string& collection, const std::string& content) {
    // 1. 向量化内容
    auto embedResult = ollama_.embed(content);
    if (embedResult.isError()) {
        return makeError<bool>(embedResult.error().code,
            "Failed to embed content: " + embedResult.error().message);
    }
    
    // 2. 搜索相似度接近 1.0 的记录
    auto searchResult = qdrant_.search(collection, embedResult.value(), 10, 0.99f);
    if (searchResult.isError()) {
        return makeError<bool>(searchResult.error().code,
            "Failed to search: " + searchResult.error().message);
    }
    
    // 3. 查找完全匹配的 content
    for (const auto& sr : searchResult.value()) {
        if (sr.payload.contains("content") && 
            sr.payload["content"].get<std::string>() == content) {
            // 找到完全匹配的内容，删除该记录
            auto deleteResult = qdrant_.deletePoints(collection, {sr.id});
            if (deleteResult.isError()) {
                return makeError<bool>(deleteResult.error().code,
                    "Failed to delete: " + deleteResult.error().message);
            }
            return makeSuccess(true);  // 删除成功
        }
    }
    
    // 没有找到完全匹配的内容
    return makeSuccess(false);
}

Result<bool> MemoryService::update(const UpdateMemoryRequest& req) {
    // 1. 获取现有记忆
    auto getResult = qdrant_.getPoint(req.collection, req.id);
    if (getResult.isError()) {
        return makeError<bool>(getResult.error().code, getResult.error().message);
    }
    
    auto existing = getResult.value();
    if (!existing.has_value()) {
        return makeError<bool>(ErrorCode::POINT_NOT_FOUND,
            "Memory not found: " + req.id);
    }
    
    // 2. 构建更新的 point
    Point point;
    point.id = req.id;
    
    // 更新内容（如果有）
    std::string content;
    if (req.content.has_value()) {
        content = *req.content;
        // 重新向量化
        auto embedResult = ollama_.embed(content);
        if (embedResult.isError()) {
            return makeError<bool>(embedResult.error().code,
                "Failed to embed new content: " + embedResult.error().message);
        }
        point.vector = embedResult.value();
    } else {
        content = existing->payload.value("content", "");
        point.vector = existing->vector;
    }
    
    // 3. 构建 payload
    auto payload = existing->payload;
    payload["content"] = content;
    
    if (req.type.has_value()) {
        payload["type"] = *req.type;
    }
    
    // 更新标签
    if (payload.contains("tags")) {
        std::vector<std::string> currentTags = payload["tags"].get<std::vector<std::string>>();
        
        // 移除标签
        for (const auto& tag : req.removeTags) {
            auto it = std::remove(currentTags.begin(), currentTags.end(), tag);
            currentTags.erase(it, currentTags.end());
        }
        
        // 添加标签
        for (const auto& tag : req.addTags) {
            if (std::find(currentTags.begin(), currentTags.end(), tag) == currentTags.end()) {
                currentTags.push_back(tag);
            }
        }
        
        payload["tags"] = currentTags;
    } else if (!req.addTags.empty()) {
        payload["tags"] = req.addTags;
    }
    
    // 更新时间戳
    auto now = std::chrono::system_clock::now();
    payload["updated_at"] = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    point.payload = payload;
    
    // 4. 保存更新
    return qdrant_.upsert(req.collection, {point});
}

Result<ImportStats> MemoryService::importFile(const ImportRequest& req) {
    ImportStats stats;
    
    // 1. 读取文件
    auto readResult = FileReader::readAll(req.filePath);
    if (readResult.isError()) {
        return makeError<ImportStats>(readResult.error().code, 
            "Failed to read file: " + readResult.error().message);
    }
    
    // 2. 分块
    TextChunker chunker(req.chunkOptions);
    auto chunks = chunker.chunk(readResult.value());
    stats.totalChunks = chunks.size();
    
    // 3. 确保集合存在
    auto ensureResult = ensureCollection(req.collection);
    if (ensureResult.isError()) {
        return makeError<ImportStats>(ensureResult.error().code,
            "Failed to ensure collection: " + ensureResult.error().message);
    }
    
    // 4. 批量处理
    std::vector<Point> points;
    for (size_t i = 0; i < chunks.size(); ++i) {
        // 向量化
        auto embedResult = ollama_.embed(chunks[i]);
        if (embedResult.isError()) {
            stats.failedCount++;
            stats.errors.push_back("Chunk " + std::to_string(i) + ": " + embedResult.error().message);
            continue;
        }
        
        // 构建 point
        Point point;
        point.id = UUID::generate();
        point.vector = embedResult.value();
        
        auto payload = buildPayload(chunks[i]);
        point.payload = payload;
        
        points.push_back(point);
        
        // 批量插入（每 10 个或最后一批）
        if (points.size() >= 10 || i == chunks.size() - 1) {
            auto upsertResult = qdrant_.upsert(req.collection, points);
            if (upsertResult.isSuccess()) {
                stats.successCount += points.size();
            } else {
                stats.failedCount += points.size();
                stats.errors.push_back(upsertResult.error().message);
            }
            points.clear();
        }
    }
    
    return makeSuccess(stats);
}

Result<std::optional<FormattedSearchResult>> MemoryService::get(
    const std::string& collection, const std::string& id) {
    
    auto result = qdrant_.getPoint(collection, id);
    if (result.isError()) {
        return makeError<std::optional<FormattedSearchResult>>(result.error().code,
            result.error().message);
    }
    
    auto point = result.value();
    if (!point.has_value()) {
        return makeSuccess<std::optional<FormattedSearchResult>>(std::nullopt);
    }
    
    SearchResult sr;
    sr.id = point->id;
    sr.score = 1.0f;
    sr.payload = point->payload;
    
    return makeSuccess<std::optional<FormattedSearchResult>>(formatResult(sr));
}

Result<int64_t> MemoryService::count(const std::string& collection,
                                      const std::optional<std::string>& type,
                                      const std::vector<std::string>& tags) {
    Filter filter;
    if (type.has_value()) {
        filter.type = type;
    }
    if (!tags.empty()) {
        filter.tags = tags;
    }
    
    return qdrant_.countPoints(collection, filter);
}

json MemoryService::buildPayload(const std::string& content) {
    json payload;
    payload["content"] = content;
    
    // 添加日期时间，格式：YYYYMMDD HH:MM:SS
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_t_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y/%m/%d %H:%M:%S");
    payload["datetime"] = oss.str();
    
    return payload;
}

FormattedSearchResult MemoryService::formatResult(const SearchResult& result) {
    FormattedSearchResult fsr;
    fsr.id = result.id;
    fsr.score = result.score;
    
    if (result.payload.contains("content")) {
        fsr.content = result.payload["content"].get<std::string>();
    }
    
    if (result.payload.contains("type")) {
        fsr.type = result.payload["type"].get<std::string>();
    }
    
    if (result.payload.contains("tags")) {
        fsr.tags = result.payload["tags"].get<std::vector<std::string>>();
    }
    
    if (result.payload.contains("timestamp")) {
        fsr.timestamp = result.payload["timestamp"].get<int64_t>();
    } else {
        fsr.timestamp = 0;
    }
    
    if (result.payload.contains("datetime")) {
        fsr.datetime = result.payload["datetime"].get<std::string>();
    }
    
    return fsr;
}

Result<bool> MemoryService::ensureCollection(const std::string& name) {
    // 检查集合是否存在
    auto existsResult = qdrant_.collectionExists(name);
    if (existsResult.isError()) {
        return makeError<bool>(existsResult.error().code, existsResult.error().message);
    }
    
    if (existsResult.value()) {
        return makeSuccess(true);
    }
    
    // 创建集合
    auto& config = ConfigManager::getInstance().getConfig();
    return qdrant_.createCollection(name, config.embeddingDimension);
}

} // namespace qdrant
