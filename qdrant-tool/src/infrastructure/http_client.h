#pragma once

#include "result.h"
#include "../../include/third_party/json.hpp"
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>

namespace qdrant {

using json = nlohmann::json;

// HTTP 响应结构
struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
    
    bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }
    bool isClientError() const { return statusCode >= 400 && statusCode < 500; }
    bool isServerError() const { return statusCode >= 500 && statusCode < 600; }
    
    json jsonBody() const {
        if (body.empty()) return json::object();
        return json::parse(body, nullptr, false);
    }
};

// 查询参数类型
using QueryParams = std::map<std::string, std::string>;

// HTTP 客户端类
class HttpClient {
public:
    explicit HttpClient(const std::string& baseUrl);
    ~HttpClient();
    
    // 禁止拷贝，允许移动
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;
    
    // 设置超时（毫秒）
    void setTimeout(int connectMs, int totalMs);
    
    // HTTP 方法
    Result<HttpResponse> get(const std::string& path, const QueryParams& params = {});
    Result<HttpResponse> post(const std::string& path, const json& body = json::object());
    Result<HttpResponse> put(const std::string& path, const json& body = json::object());
    Result<HttpResponse> del(const std::string& path);
    
    // 设置请求头
    void setHeader(const std::string& key, const std::string& value);
    void clearHeaders();
    
private:
    std::string baseUrl_;
    CURL* curl_;
    curl_slist* headers_;
    int connectTimeoutMs_;
    int totalTimeoutMs_;
    
    // 执行请求的通用方法
    Result<HttpResponse> performRequest(const std::string& url);
    
    // 构建完整 URL
    std::string buildUrl(const std::string& path, const QueryParams& params = {}) const;
    
    // CURL 写回调
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    
    // 初始化 CURL
    void initCurl();
    void cleanupCurl();
};

} // namespace qdrant
