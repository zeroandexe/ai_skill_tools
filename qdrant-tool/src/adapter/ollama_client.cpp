#include "ollama_client.h"

namespace qdrant {

OllamaClient::OllamaClient(const std::string& baseUrl, 
                            const std::string& model,
                            int dimension)
    : http_(baseUrl)
    , model_(model)
    , dimension_(dimension) {}

Result<bool> OllamaClient::health() {
    auto result = http_.get("");
    if (result.isError()) {
        return makeError<bool>(ErrorCode::OLLAMA_CONNECTION_FAILED,
            "Failed to connect to Ollama: " + result.error().message);
    }
    
    auto response = result.value();
    return makeSuccess(response.isSuccess());
}

Result<std::vector<float>> OllamaClient::embed(const std::string& text) {
    auto batchResult = embedBatch({text});
    if (batchResult.isError()) {
        return makeError<std::vector<float>>(batchResult.error().code,
            batchResult.error().message);
    }
    
    auto embeddings = batchResult.value();
    if (embeddings.empty()) {
        return makeError<std::vector<float>>(ErrorCode::EMBEDDING_FAILED,
            "Empty embedding result");
    }
    
    return makeSuccess(embeddings[0]);
}

Result<std::vector<std::vector<float>>> OllamaClient::embedBatch(
    const std::vector<std::string>& texts) {
    
    if (texts.empty()) {
        return makeSuccess<std::vector<std::vector<float>>>({});
    }
    
    json body = buildEmbedRequest(texts);
    
    auto result = http_.post("api/embed", body);
    if (result.isError()) {
        return makeError<std::vector<std::vector<float>>>(ErrorCode::OLLAMA_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (!response.isSuccess()) {
        return makeError<std::vector<std::vector<float>>>(ErrorCode::EMBEDDING_FAILED,
            "Failed to generate embeddings: " + response.body);
    }
    
    try {
        auto jsonBody = response.jsonBody();
        
        if (!jsonBody.contains("embeddings")) {
            return makeError<std::vector<std::vector<float>>>(ErrorCode::EMBEDDING_FAILED,
                "Response does not contain embeddings");
        }
        
        std::vector<std::vector<float>> embeddings;
        for (const auto& emb : jsonBody["embeddings"]) {
            embeddings.push_back(emb.get<std::vector<float>>());
        }
        
        // 验证维度
        for (size_t i = 0; i < embeddings.size(); ++i) {
            if (static_cast<int>(embeddings[i].size()) != dimension_) {
                return makeError<std::vector<std::vector<float>>>(ErrorCode::EMBEDDING_FAILED,
                    "Unexpected embedding dimension: " + std::to_string(embeddings[i].size()) +
                    " (expected " + std::to_string(dimension_) + ")");
            }
        }
        
        return makeSuccess(embeddings);
    } catch (const std::exception& e) {
        return makeError<std::vector<std::vector<float>>>(ErrorCode::EMBEDDING_FAILED,
            "Failed to parse embedding response: " + std::string(e.what()));
    }
}

Result<json> OllamaClient::getModelInfo() {
    json body = {{"name", model_}};
    
    auto result = http_.post("api/show", body);
    if (result.isError()) {
        return makeError<json>(ErrorCode::OLLAMA_CONNECTION_FAILED,
            result.error().message);
    }
    
    auto response = result.value();
    if (!response.isSuccess()) {
        return makeError<json>(ErrorCode::INTERNAL_ERROR,
            "Failed to get model info: " + response.body);
    }
    
    try {
        return makeSuccess(response.jsonBody());
    } catch (const std::exception& e) {
        return makeError<json>(ErrorCode::INTERNAL_ERROR,
            "Failed to parse model info: " + std::string(e.what()));
    }
}

json OllamaClient::buildEmbedRequest(const std::vector<std::string>& texts) const {
    json body = {
        {"model", model_},
        {"input", texts}
    };
    
    return body;
}

} // namespace qdrant
