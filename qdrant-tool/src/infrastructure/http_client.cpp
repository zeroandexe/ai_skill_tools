#include "http_client.h"
#include <cstring>

namespace qdrant {

HttpClient::HttpClient(const std::string& baseUrl) 
    : baseUrl_(baseUrl)
    , curl_(nullptr)
    , headers_(nullptr)
    , connectTimeoutMs_(5000)
    , totalTimeoutMs_(30000) {
    initCurl();
}

HttpClient::~HttpClient() {
    cleanupCurl();
}

HttpClient::HttpClient(HttpClient&& other) noexcept
    : baseUrl_(std::move(other.baseUrl_))
    , curl_(other.curl_)
    , headers_(other.headers_)
    , connectTimeoutMs_(other.connectTimeoutMs_)
    , totalTimeoutMs_(other.totalTimeoutMs_) {
    other.curl_ = nullptr;
    other.headers_ = nullptr;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        cleanupCurl();
        baseUrl_ = std::move(other.baseUrl_);
        curl_ = other.curl_;
        headers_ = other.headers_;
        connectTimeoutMs_ = other.connectTimeoutMs_;
        totalTimeoutMs_ = other.totalTimeoutMs_;
        other.curl_ = nullptr;
        other.headers_ = nullptr;
    }
    return *this;
}

void HttpClient::setTimeout(int connectMs, int totalMs) {
    connectTimeoutMs_ = connectMs;
    totalTimeoutMs_ = totalMs;
}

void HttpClient::setHeader(const std::string& key, const std::string& value) {
    std::string header = key + ": " + value;
    headers_ = curl_slist_append(headers_, header.c_str());
}

void HttpClient::clearHeaders() {
    if (headers_) {
        curl_slist_free_all(headers_);
        headers_ = nullptr;
    }
}

Result<HttpResponse> HttpClient::get(const std::string& path, const QueryParams& params) {
    if (!curl_) {
        return makeError<HttpResponse>(ErrorCode::INTERNAL_ERROR, "CURL not initialized");
    }
    
    std::string url = buildUrl(path, params);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_, CURLOPT_POST, 0L);
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, nullptr);
    
    // 设置默认 JSON 请求头
    clearHeaders();
    setHeader("Content-Type", "application/json");
    setHeader("Accept", "application/json");
    
    return performRequest(url);
}

Result<HttpResponse> HttpClient::post(const std::string& path, const json& body) {
    if (!curl_) {
        return makeError<HttpResponse>(ErrorCode::INTERNAL_ERROR, "CURL not initialized");
    }
    
    std::string url = buildUrl(path);
    std::string postData = body.dump();
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, postData.length());
    
    // 设置请求头
    clearHeaders();
    setHeader("Content-Type", "application/json");
    setHeader("Accept", "application/json");
    
    return performRequest(url);
}

Result<HttpResponse> HttpClient::put(const std::string& path, const json& body) {
    if (!curl_) {
        return makeError<HttpResponse>(ErrorCode::INTERNAL_ERROR, "CURL not initialized");
    }
    
    std::string url = buildUrl(path);
    std::string putData = body.dump();
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 0L);
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, putData.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, putData.length());
    
    // 设置请求头
    clearHeaders();
    setHeader("Content-Type", "application/json");
    setHeader("Accept", "application/json");
    
    return performRequest(url);
}

Result<HttpResponse> HttpClient::del(const std::string& path) {
    if (!curl_) {
        return makeError<HttpResponse>(ErrorCode::INTERNAL_ERROR, "CURL not initialized");
    }
    
    std::string url = buildUrl(path);
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 0L);
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    
    // 设置请求头
    clearHeaders();
    setHeader("Content-Type", "application/json");
    setHeader("Accept", "application/json");
    
    return performRequest(url);
}

Result<HttpResponse> HttpClient::performRequest(const std::string& url) {
    (void)url;  // url 通过 CURLOPT_URL 在调用前设置
    std::string responseBody;
    
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, connectTimeoutMs_);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, totalTimeoutMs_);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 5L);
    
    CURLcode res = curl_easy_perform(curl_);
    
    if (res != CURLE_OK) {
        std::string errorMsg = curl_easy_strerror(res);
        return makeError<HttpResponse>(ErrorCode::INTERNAL_ERROR, 
            "HTTP request failed: " + errorMsg);
    }
    
    HttpResponse response;
    response.body = responseBody;
    
    long httpCode = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);
    response.statusCode = static_cast<int>(httpCode);
    
    return makeSuccess(response);
}

std::string HttpClient::buildUrl(const std::string& path, const QueryParams& params) const {
    std::string url = baseUrl_;
    
    // 确保 baseUrl 以 / 结尾，path 不以 / 开头
    if (!url.empty() && url.back() != '/' && !path.empty() && path[0] != '/') {
        url += '/';
    }
    url += path;
    
    // 添加查询参数
    if (!params.empty()) {
        url += "?";
        bool first = true;
        for (const auto& [key, value] : params) {
            if (!first) url += "&";
            url += key + "=" + value;  // 注意：实际应该进行 URL 编码
            first = false;
        }
    }
    
    return url;
}

size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void HttpClient::initCurl() {
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

void HttpClient::cleanupCurl() {
    if (headers_) {
        curl_slist_free_all(headers_);
        headers_ = nullptr;
    }
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

} // namespace qdrant
