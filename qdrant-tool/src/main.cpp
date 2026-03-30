#include "infrastructure/config_manager.h"
#include "infrastructure/result.h"
#include "adapter/qdrant_client.h"
#include "adapter/ollama_client.h"
#include "service/memory_service.h"
#include "service/collection_service.h"
#include "cli/argument_parser.h"
#include "cli/command_router.h"
#include <iostream>
#include <string>

// 版本信息
#define QDRANT_TOOL_VERSION "1.0.0"

using namespace qdrant;

int main(int argc, char** argv) {
    // 1. 加载配置
    auto& configManager = ConfigManager::getInstance();
    configManager.load(argc, argv);
    const auto& config = configManager.getConfig();
    
    // 2. 解析命令行参数
    ArgumentParser parser;
    ParsedArgs args = parser.parse(argc, argv);
    
    // 3. 处理帮助和版本
    if (args.help) {
        if (args.command == CommandType::UNKNOWN) {
            std::cout << parser.getHelp() << std::endl;
        } else {
            std::cout << parser.getCommandHelp(args.command) << std::endl;
        }
        return 0;
    }
    
    if (args.version) {
        std::cout << "qdrant-tool version " << QDRANT_TOOL_VERSION << std::endl;
        return 0;
    }
    
    // 显示当前配置
    if (argc > 1 && std::string(argv[1]) == "--show-config") {
        std::cout << configManager.toString() << std::endl;
        return 0;
    }
    
    // 4. 验证参数
    auto validationError = parser.validate(args);
    if (validationError.has_value()) {
        json error;
        error["success"] = false;
        error["error"] = {
            {"code", "E001"},
            {"message", validationError.value()}
        };
        std::cout << error.dump(2) << std::endl;
        return 1;
    }
    
    // 5. 初始化客户端
    try {
        QdrantClient qdrantClient(config.qdrantUrl);
        OllamaClient ollamaClient(config.ollamaUrl, config.embeddingModel, 
                                   config.embeddingDimension);
        
        MemoryService memoryService(qdrantClient, ollamaClient);
        CollectionService collectionService(qdrantClient);
        
        // 6. 路由并执行命令
        CommandRouter router(memoryService, collectionService);
        return router.execute(args);
        
    } catch (const std::exception& e) {
        json error;
        error["success"] = false;
        error["error"] = {
            {"code", "E999"},
            {"message", std::string("Initialization failed: ") + e.what()}
        };
        std::cout << error.dump(2) << std::endl;
        return 1;
    }
}
