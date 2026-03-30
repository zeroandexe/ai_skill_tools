#pragma once

#include "argument_parser.h"
#include "../service/memory_service.h"
#include "../service/collection_service.h"

namespace qdrant {

// 命令路由器类
class CommandRouter {
public:
    CommandRouter(MemoryService& memoryService, CollectionService& collectionService);
    ~CommandRouter() = default;
    
    // 执行命令
    int execute(const ParsedArgs& args);
    
    // 获取最后错误信息
    const std::string& getLastError() const { return lastError_; }
    
private:
    MemoryService& memoryService_;
    CollectionService& collectionService_;
    std::string lastError_;
    
    // 各命令执行函数
    int executeAdd(const ParsedArgs& args);
    int executeSearch(const ParsedArgs& args);
    int executeDelete(const ParsedArgs& args);
    int executeUpdate(const ParsedArgs& args);
    int executeImport(const ParsedArgs& args);
    int executeCollectionCreate(const ParsedArgs& args);
    int executeCollectionList(const ParsedArgs& args);
    int executeCollectionInfo(const ParsedArgs& args);
    int executeCollectionDelete(const ParsedArgs& args);
    int executeHealth(const ParsedArgs& args);
    int executeEval(const ParsedArgs& args);
    
    // 辅助函数
    std::string getOption(const ParsedArgs& args, const std::string& name, 
                          const std::string& defaultValue = "");
    std::vector<std::string> getMultiOption(const ParsedArgs& args, const std::string& name);
    int getIntOption(const ParsedArgs& args, const std::string& name, int defaultValue);
    float getFloatOption(const ParsedArgs& args, const std::string& name, float defaultValue);
    
    // 输出格式化
    void outputSuccess(const json& data);
    void outputSuccess(const std::string& message);
    void outputError(const std::string& code, const std::string& message);
    void outputResults(const std::vector<FormattedSearchResult>& results);
};

} // namespace qdrant
