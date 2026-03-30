#include "qdrant_client.h"

namespace qdrant {

QdrantClient::QdrantClient(const std::string& baseUrl) : http_(baseUrl) {}

Result<bool> QdrantClient::health() {
    auto result = http_.get("");
    if (result.isError()) {
        return makeError<bool>(ErrorCode::QDRANT_CONNECTION_FAILED, 
            "Failed to connect to Qdrant: " + result.error().message);
    }
    
    auto response = result.value();
    return makeSuccess(response.isSuccess());
}

Result<bool> QdrantClient::createCollection(const std::string& name,
                                             int dimension,
                                             const std::string& distance) {
    json body = {
        {"vectors", {
            {"size", dimension},
            {"distance", distance}
        }}
    };
    
    auto result = http_.put("collections/" + name, body);
    if (result.isError()) {
        return makeError<bool>(ErrorCode::QDRANT_CONNECTION_FAILED, 
            result.error().message);
    }
    
    auto response = result.value();
    if (response.isSuccess()) {
        return makeSuccess(true);
    }
    
    return makeError<bool>(ErrorCode::INTERNAL_ERROR,
        "Failed to create collection: " + response.body);
}

Result<bool> QdrantClient::deleteCollection(const std::string& name) {
    auto result = http_.del("collections/" + name);
    if (result.isError()) {
        return makeError<bool>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.isSuccess()) {
        return makeSuccess(true);
    }
    
    if (response.statusCode == 404) {
        return makeError<bool>(ErrorCode::COLLECTION_NOT_FOUND,
            "Collection not found: " + name);
    }
    
    return makeError<bool>(ErrorCode::INTERNAL_ERROR,
        "Failed to delete collection: " + response.body);
}

Result<std::vector<std::string>> QdrantClient::listCollections() {
    auto result = http_.get("collections");
    if (result.isError()) {
        return makeError<std::vector<std::string>>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (!response.isSuccess()) {
        return makeError<std::vector<std::string>>(ErrorCode::INTERNAL_ERROR,
            "Failed to list collections: " + response.body);
    }
    
    try {
        auto jsonBody = response.jsonBody();
        std::vector<std::string> collections;
        
        for (const auto& coll : jsonBody["result"]["collections"]) {
            collections.push_back(coll["name"].get<std::string>());
        }
        
        return makeSuccess(collections);
    } catch (const std::exception& e) {
        return makeError<std::vector<std::string>>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse collections response: " + std::string(e.what()));
    }
}

Result<json> QdrantClient::getCollectionInfo(const std::string& name) {
    auto result = http_.get("collections/" + name);
    if (result.isError()) {
        return makeError<json>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.statusCode == 404) {
        return makeError<json>(ErrorCode::COLLECTION_NOT_FOUND,
            "Collection not found: " + name);
    }
    
    if (!response.isSuccess()) {
        return makeError<json>(ErrorCode::INTERNAL_ERROR,
            "Failed to get collection info: " + response.body);
    }
    
    try {
        return makeSuccess(response.jsonBody()["result"]);
    } catch (const std::exception& e) {
        return makeError<json>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse collection info: " + std::string(e.what()));
    }
}

Result<bool> QdrantClient::collectionExists(const std::string& name) {
    auto result = getCollectionInfo(name);
    if (result.isSuccess()) {
        return makeSuccess(true);
    }
    if (result.error().code == ErrorCode::COLLECTION_NOT_FOUND) {
        return makeSuccess(false);
    }
    return makeError<bool>(result.error().code, result.error().message);
}

Result<bool> QdrantClient::upsert(const std::string& collection,
                                   const std::vector<Point>& points) {
    json batch = json::array();
    for (const auto& point : points) {
        json p = {
            {"id", point.id},
            {"vector", point.vector},
            {"payload", point.payload}
        };
        batch.push_back(p);
    }
    
    json body = {{"points", batch}};
    
    auto result = http_.put("collections/" + collection + "/points", body);
    if (result.isError()) {
        return makeError<bool>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.isSuccess()) {
        return makeSuccess(true);
    }
    
    if (response.statusCode == 404) {
        return makeError<bool>(ErrorCode::COLLECTION_NOT_FOUND,
            "Collection not found: " + collection);
    }
    
    return makeError<bool>(ErrorCode::INTERNAL_ERROR,
        "Failed to upsert points: " + response.body);
}

Result<bool> QdrantClient::deletePoints(const std::string& collection,
                                         const std::vector<std::string>& ids) {
    json body = {{"points", ids}};
    
    auto result = http_.post("collections/" + collection + "/points/delete", body);
    if (result.isError()) {
        return makeError<bool>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.isSuccess()) {
        return makeSuccess(true);
    }
    
    if (response.statusCode == 404) {
        return makeError<bool>(ErrorCode::COLLECTION_NOT_FOUND,
            "Collection not found: " + collection);
    }
    
    return makeError<bool>(ErrorCode::INTERNAL_ERROR,
        "Failed to delete points: " + response.body);
}

Result<std::vector<SearchResult>> QdrantClient::search(const std::string& collection,
                                                        const std::vector<float>& vector,
                                                        int limit,
                                                        float minScore,
                                                        const Filter& filter) {
    json body = {
        {"vector", vector},
        {"limit", limit},
        {"with_payload", true},
        {"with_vector", false}
    };
    
    if (minScore > 0.0f) {
        body["score_threshold"] = minScore;
    }
    
    if (!filter.isEmpty()) {
        body["filter"] = buildFilterJson(filter);
    }
    
    auto result = http_.post("collections/" + collection + "/points/search", body);
    if (result.isError()) {
        return makeError<std::vector<SearchResult>>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.statusCode == 404) {
        return makeError<std::vector<SearchResult>>(ErrorCode::COLLECTION_NOT_FOUND,
            "Collection not found: " + collection);
    }
    
    if (!response.isSuccess()) {
        return makeError<std::vector<SearchResult>>(ErrorCode::INTERNAL_ERROR,
            "Failed to search: " + response.body);
    }
    
    try {
        std::vector<SearchResult> results;
        auto jsonBody = response.jsonBody();
        
        for (const auto& item : jsonBody["result"]) {
            SearchResult sr;
            sr.id = item["id"].get<std::string>();
            sr.score = item["score"].get<float>();
            if (item.contains("payload")) {
                sr.payload = item["payload"];
            }
            results.push_back(sr);
        }
        
        return makeSuccess(results);
    } catch (const std::exception& e) {
        return makeError<std::vector<SearchResult>>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse search results: " + std::string(e.what()));
    }
}

Result<std::optional<Point>> QdrantClient::getPoint(const std::string& collection,
                                                     const std::string& id) {
    auto result = http_.get("collections/" + collection + "/points/" + id);
    if (result.isError()) {
        return makeError<std::optional<Point>>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.statusCode == 404) {
        return makeSuccess<std::optional<Point>>(std::nullopt);
    }
    
    if (!response.isSuccess()) {
        return makeError<std::optional<Point>>(ErrorCode::INTERNAL_ERROR,
            "Failed to get point: " + response.body);
    }
    
    try {
        auto jsonBody = response.jsonBody();
        if (jsonBody["result"].is_null()) {
            return makeSuccess<std::optional<Point>>(std::nullopt);
        }
        
        Point point;
        point.id = jsonBody["result"]["id"].get<std::string>();
        point.vector = jsonBody["result"]["vector"].get<std::vector<float>>();
        if (jsonBody["result"].contains("payload")) {
            point.payload = jsonBody["result"]["payload"];
        }
        
        return makeSuccess<std::optional<Point>>(point);
    } catch (const std::exception& e) {
        return makeError<std::optional<Point>>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse point data: " + std::string(e.what()));
    }
}

Result<int64_t> QdrantClient::countPoints(const std::string& collection,
                                           const Filter& filter) {
    json body = json::object();
    
    if (!filter.isEmpty()) {
        body["filter"] = buildFilterJson(filter);
    }
    
    auto result = http_.post("collections/" + collection + "/points/count", body);
    if (result.isError()) {
        return makeError<int64_t>(ErrorCode::QDRANT_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (response.statusCode == 404) {
        return makeError<int64_t>(ErrorCode::COLLECTION_NOT_FOUND,
            "Collection not found: " + collection);
    }
    
    if (!response.isSuccess()) {
        return makeError<int64_t>(ErrorCode::INTERNAL_ERROR,
            "Failed to count points: " + response.body);
    }
    
    try {
        auto jsonBody = response.jsonBody();
        int64_t count = jsonBody["result"]["count"].get<int64_t>();
        return makeSuccess(count);
    } catch (const std::exception& e) {
        return makeError<int64_t>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse count result: " + std::string(e.what()));
    }
}

json QdrantClient::buildFilterJson(const Filter& filter) const {
    json must = json::array();
    
    if (filter.type.has_value()) {
        must.push_back({
            {"key", "type"},
            {"match", {{"value", *filter.type}}}
        });
    }
    
    if (!filter.tags.empty()) {
        json should = json::array();
        for (const auto& tag : filter.tags) {
            should.push_back({
                {"key", "tags"},
                {"match", {{"value", tag}}}
            });
        }
        must.push_back({{"should", should}});
    }
    
    if (filter.source.has_value()) {
        must.push_back({
            {"key", "source"},
            {"match", {{"value", *filter.source}}}
        });
    }
    
    if (filter.timestampRange.has_value()) {
        must.push_back({
            {"key", "timestamp"},
            {"range", {
                {"gte", filter.timestampRange->first},
                {"lte", filter.timestampRange->second}
            }}
        });
    }
    
    return {{"must", must}};
}

Error QdrantClient::handleHttpError(const HttpResponse& response, 
                                     ErrorCode defaultCode) const {
    if (response.statusCode == 404) {
        return Error(ErrorCode::COLLECTION_NOT_FOUND, "Resource not found");
    }
    return Error(defaultCode, "HTTP error: " + std::to_string(response.statusCode));
}

} // namespace qdrant
