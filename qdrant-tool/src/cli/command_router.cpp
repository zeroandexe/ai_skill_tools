#include "command_router.h"
#include <iostream>
#include <iomanip>

namespace qdrant {

CommandRouter::CommandRouter(MemoryService& memoryService, 
                              CollectionService& collectionService)
    : memoryService_(memoryService)
    , collectionService_(collectionService) {}

int CommandRouter::execute(const ParsedArgs& args) {
    switch (args.command) {
        case CommandType::ADD:
            return executeAdd(args);
        case CommandType::SEARCH:
            return executeSearch(args);
        case CommandType::DELETE:
            return executeDelete(args);
        case CommandType::UPDATE:
            return executeUpdate(args);
        case CommandType::IMPORT:
            return executeImport(args);
        case CommandType::COLLECTION_CREATE:
            return executeCollectionCreate(args);
        case CommandType::COLLECTION_LIST:
            return executeCollectionList(args);
        case CommandType::COLLECTION_INFO:
            return executeCollectionInfo(args);
        case CommandType::COLLECTION_DELETE:
            return executeCollectionDelete(args);
        case CommandType::HEALTH:
            return executeHealth(args);
        case CommandType::EVAL:
            return executeEval(args);
        default:
            outputError("E001", "Unknown command");
            return 1;
    }
}

int CommandRouter::executeAdd(const ParsedArgs& args) {
    AddMemoryRequest req;
    req.collection = getOption(args, "collection", "memory");
    req.content = getOption(args, "content", "");
    
    if (req.content.empty()) {
        outputError("E001", "Content is required");
        return 1;
    }
    
    std::string id = getOption(args, "id", "");
    if (!id.empty()) {
        req.id = id;
    }
    
    std::string type = getOption(args, "type", "");
    if (!type.empty()) {
        req.type = type;
    }
    
    req.tags = getMultiOption(args, "tag");
    
    std::string source = getOption(args, "source", "");
    if (!source.empty()) {
        req.source = source;
    }
    
    auto result = memoryService_.add(req);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    response["id"] = result.value();
    outputSuccess(response);
    
    return 0;
}

int CommandRouter::executeSearch(const ParsedArgs& args) {
    SearchRequest req;
    req.collection = getOption(args, "collection", "memory");
    req.query = getOption(args, "query", "");
    
    if (req.query.empty()) {
        outputError("E001", "Query is required");
        return 1;
    }
    
    req.limit = getIntOption(args, "limit", 10);
    req.minScore = getFloatOption(args, "min-score", 0.0f);
    
    std::string filterType = getOption(args, "filter-type", "");
    if (!filterType.empty()) {
        req.filterType = filterType;
    }
    
    req.filterTags = getMultiOption(args, "filter-tag");
    req.withContent = getOption(args, "with-content", "false") == "true";
    
    auto result = memoryService_.search(req);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    response["count"] = result.value().size();
    response["results"] = json::array();
    
    for (const auto& r : result.value()) {
        json item;
        item["id"] = r.id;
        item["score"] = r.score;
        item["content"] = r.content;
        item["datetime"] = r.datetime;
        response["results"].push_back(item);
    }
    
    outputSuccess(response);
    return 0;
}

int CommandRouter::executeDelete(const ParsedArgs& args) {
    std::string collection = getOption(args, "collection", "memory");
    std::string id = getOption(args, "id", "");
    std::string content = getOption(args, "content", "");
    
    // 必须提供 id 或 content 之一
    if (id.empty() && content.empty()) {
        outputError("E001", "Either --id or --content is required");
        return 1;
    }
    
    // 如果提供了 id，按 id 删除
    if (!id.empty()) {
        auto result = memoryService_.remove(collection, id);
        if (result.isError()) {
            outputError(result.error().codeString(), result.error().message);
            return 1;
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Memory deleted successfully";
        outputSuccess(response);
        return 0;
    }
    
    // 如果提供了 content，按内容删除
    auto result = memoryService_.removeByContent(collection, content);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    if (result.value()) {
        response["message"] = "Memory deleted successfully";
    } else {
        response["message"] = "No matching memory found";
    }
    outputSuccess(response);
    
    return 0;
}

int CommandRouter::executeUpdate(const ParsedArgs& args) {
    UpdateMemoryRequest req;
    req.collection = getOption(args, "collection", "memory");
    req.id = getOption(args, "id", "");
    
    if (req.id.empty()) {
        outputError("E001", "ID is required");
        return 1;
    }
    
    std::string content = getOption(args, "content", "");
    if (!content.empty()) {
        req.content = content;
    }
    
    std::string type = getOption(args, "type", "");
    if (!type.empty()) {
        req.type = type;
    }
    
    req.addTags = getMultiOption(args, "add-tag");
    req.removeTags = getMultiOption(args, "remove-tag");
    
    auto result = memoryService_.update(req);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    response["message"] = "Memory updated successfully";
    outputSuccess(response);
    
    return 0;
}

int CommandRouter::executeImport(const ParsedArgs& args) {
    ImportRequest req;
    req.filePath = getOption(args, "file", "");
    
    if (req.filePath.empty()) {
        outputError("E001", "File path is required");
        return 1;
    }
    
    req.collection = getOption(args, "collection", "");
    if (req.collection.empty()) {
        // 使用文件名作为集合名
        size_t lastSlash = req.filePath.find_last_of('/');
        size_t lastDot = req.filePath.find_last_of('.');
        
        if (lastDot != std::string::npos && 
            (lastSlash == std::string::npos || lastDot > lastSlash)) {
            req.collection = req.filePath.substr(
                lastSlash == std::string::npos ? 0 : lastSlash + 1,
                lastDot - (lastSlash == std::string::npos ? 0 : lastSlash + 1));
        } else {
            req.collection = "imported";
        }
    }
    
    req.chunkOptions.chunkSize = static_cast<size_t>(getIntOption(args, "chunk-size", 500));
    req.chunkOptions.overlap = static_cast<size_t>(getIntOption(args, "chunk-overlap", 50));
    
    std::string type = getOption(args, "type", "");
    if (!type.empty()) {
        req.type = type;
    }
    
    req.tags = getMultiOption(args, "tag");
    req.source = req.filePath;
    
    auto result = memoryService_.importFile(req);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    const auto& stats = result.value();
    json response;
    response["success"] = true;
    response["total_chunks"] = stats.totalChunks;
    response["success_count"] = stats.successCount;
    response["failed_count"] = stats.failedCount;
    if (!stats.errors.empty()) {
        response["errors"] = stats.errors;
    }
    
    outputSuccess(response);
    return 0;
}

int CommandRouter::executeCollectionCreate(const ParsedArgs& args) {
    std::string name = getOption(args, "name", "");
    
    if (name.empty()) {
        outputError("E001", "Collection name is required");
        return 1;
    }
    
    int dimension = getIntOption(args, "dimension", 1024);
    std::string distance = getOption(args, "distance", "Cosine");
    
    auto result = collectionService_.create(name, dimension, distance);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    response["message"] = "Collection created successfully";
    outputSuccess(response);
    
    return 0;
}

int CommandRouter::executeCollectionList(const ParsedArgs& /*args*/) {
    auto result = collectionService_.list();
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    response["collections"] = result.value();
    outputSuccess(response);
    
    return 0;
}

int CommandRouter::executeCollectionInfo(const ParsedArgs& args) {
    std::string name = getOption(args, "name", "");
    
    if (name.empty()) {
        outputError("E001", "Collection name is required");
        return 1;
    }
    
    auto result = collectionService_.info(name);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    const auto& info = result.value();
    json response;
    response["success"] = true;
    response["name"] = info.name;
    response["dimension"] = info.dimension;
    response["points_count"] = info.pointsCount;
    response["distance"] = info.distance;
    
    outputSuccess(response);
    return 0;
}

int CommandRouter::executeCollectionDelete(const ParsedArgs& args) {
    std::string name = getOption(args, "name", "");
    
    if (name.empty()) {
        outputError("E001", "Collection name is required");
        return 1;
    }
    
    auto result = collectionService_.remove(name);
    if (result.isError()) {
        outputError(result.error().codeString(), result.error().message);
        return 1;
    }
    
    json response;
    response["success"] = true;
    response["message"] = "Collection deleted successfully";
    outputSuccess(response);
    
    return 0;
}

int CommandRouter::executeHealth(const ParsedArgs& /*args*/) {
    // 这里简化处理，只返回成功
    // 实际应该检查 Qdrant 和 Ollama 连接
    json response;
    response["success"] = true;
    response["qdrant"] = "unknown";
    response["ollama"] = "unknown";
    
    outputSuccess(response);
    return 0;
}

int CommandRouter::executeEval(const ParsedArgs& args) {
    std::string content = getOption(args, "content", "");
    
    if (content.empty()) {
        outputError("E001", "Content is required");
        return 1;
    }
    
    // 简化的评估逻辑
    // 实际应该调用 LLM 或复杂的启发式算法
    json response;
    response["success"] = true;
    response["score"] = 0.7;
    response["reason"] = "Contains potentially valuable information";
    response["recommendation"] = "save";
    
    outputSuccess(response);
    return 0;
}

std::string CommandRouter::getOption(const ParsedArgs& args, const std::string& name,
                                      const std::string& defaultValue) {
    auto it = args.options.find(name);
    if (it != args.options.end()) {
        return it->second;
    }
    return defaultValue;
}

std::vector<std::string> CommandRouter::getMultiOption(const ParsedArgs& args, 
                                                        const std::string& name) {
    std::vector<std::string> values;
    
    for (const auto& [key, value] : args.options) {
        if (key == name) {
            values.push_back(value);
        }
    }
    
    return values;
}

int CommandRouter::getIntOption(const ParsedArgs& args, const std::string& name, 
                                 int defaultValue) {
    std::string value = getOption(args, name, "");
    if (value.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

float CommandRouter::getFloatOption(const ParsedArgs& args, const std::string& name,
                                     float defaultValue) {
    std::string value = getOption(args, name, "");
    if (value.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stof(value);
    } catch (...) {
        return defaultValue;
    }
}

void CommandRouter::outputSuccess(const json& data) {
    std::cout << data.dump(2) << std::endl;
}

void CommandRouter::outputSuccess(const std::string& message) {
    json response;
    response["success"] = true;
    response["message"] = message;
    std::cout << response.dump(2) << std::endl;
}

void CommandRouter::outputError(const std::string& code, const std::string& message) {
    json response;
    response["success"] = false;
    response["error"] = {
        {"code", code},
        {"message", message}
    };
    std::cout << response.dump(2) << std::endl;
}

void CommandRouter::outputResults(const std::vector<FormattedSearchResult>& results) {
    for (const auto& r : results) {
        std::cout << "ID: " << r.id << std::endl;
        std::cout << "Score: " << std::fixed << std::setprecision(4) << r.score << std::endl;
        if (!r.content.empty()) {
            std::cout << "Content: " << r.content << std::endl;
        }
        std::cout << "---" << std::endl;
    }
}

} // namespace qdrant
