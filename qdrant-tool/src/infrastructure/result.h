#pragma once

#include <string>
#include <variant>
#include <optional>

namespace qdrant {

// 错误码定义
enum class ErrorCode {
    SUCCESS = 0,
    INVALID_ARGUMENT = 1,
    QDRANT_CONNECTION_FAILED = 2,
    OLLAMA_CONNECTION_FAILED = 3,
    COLLECTION_NOT_FOUND = 4,
    POINT_NOT_FOUND = 5,
    EMBEDDING_FAILED = 6,
    FILE_READ_FAILED = 7,
    INTERNAL_ERROR = 999
};

// 错误信息结构
struct Error {
    ErrorCode code;
    std::string message;
    
    Error(ErrorCode c, std::string m) : code(c), message(std::move(m)) {}
    
    std::string codeString() const {
        switch (code) {
            case ErrorCode::SUCCESS: return "E000";
            case ErrorCode::INVALID_ARGUMENT: return "E001";
            case ErrorCode::QDRANT_CONNECTION_FAILED: return "E002";
            case ErrorCode::OLLAMA_CONNECTION_FAILED: return "E003";
            case ErrorCode::COLLECTION_NOT_FOUND: return "E004";
            case ErrorCode::POINT_NOT_FOUND: return "E005";
            case ErrorCode::EMBEDDING_FAILED: return "E006";
            case ErrorCode::FILE_READ_FAILED: return "E007";
            case ErrorCode::INTERNAL_ERROR: return "E999";
            default: return "E999";
        }
    }
};

// Result 类型：表示操作结果，成功时包含值，失败时包含错误
template<typename T>
class Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error error) : data_(std::move(error)) {}
    
    bool isSuccess() const { return std::holds_alternative<T>(data_); }
    bool isError() const { return std::holds_alternative<Error>(data_); }
    
    T& value() { return std::get<T>(data_); }
    const T& value() const { return std::get<T>(data_); }
    
    Error& error() { return std::get<Error>(data_); }
    const Error& error() const { return std::get<Error>(data_); }
    
    T valueOr(T defaultValue) const {
        if (isSuccess()) return value();
        return defaultValue;
    }
    
private:
    std::variant<T, Error> data_;
};

// 特化版本：void 类型的 Result
template<>
class Result<void> {
public:
    Result() : error_(std::nullopt) {}
    Result(Error error) : error_(std::move(error)) {}
    
    bool isSuccess() const { return !error_.has_value(); }
    bool isError() const { return error_.has_value(); }
    
    Error& error() { return error_.value(); }
    const Error& error() const { return error_.value(); }
    
private:
    std::optional<Error> error_;
};

// 辅助函数
template<typename T>
Result<T> makeSuccess(T value) {
    return Result<T>(std::move(value));
}

inline Result<void> makeSuccess() {
    return Result<void>();
}

template<typename T>
Result<T> makeError(ErrorCode code, const std::string& message) {
    return Result<T>(Error(code, message));
}

inline Result<void> makeError(ErrorCode code, const std::string& message) {
    return Result<void>(Error(code, message));
}

} // namespace qdrant
