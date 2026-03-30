#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace qdrant {

// 命令类型枚举
enum class CommandType {
    UNKNOWN,
    ADD,
    SEARCH,
    DELETE,
    UPDATE,
    IMPORT,
    COLLECTION_CREATE,
    COLLECTION_LIST,
    COLLECTION_INFO,
    COLLECTION_DELETE,
    HEALTH,
    EVAL,
    HELP,
    VERSION
};

// 解析后的参数
struct ParsedArgs {
    CommandType command = CommandType::UNKNOWN;
    std::string subcommand;  // 用于 collection 的子命令
    std::map<std::string, std::string> options;
    std::vector<std::string> positional;
    bool help = false;
    bool version = false;
};

// 参数定义
struct ArgDef {
    std::string name;
    std::string shortName;
    std::string description;
    bool required = false;
    bool hasValue = true;
    std::string defaultValue;
};

// 命令定义
struct CommandDef {
    std::string name;
    std::string description;
    std::vector<ArgDef> args;
};

// 参数解析器类
class ArgumentParser {
public:
    ArgumentParser();
    ~ArgumentParser() = default;
    
    // 解析命令行参数
    ParsedArgs parse(int argc, char** argv);
    
    // 获取帮助文本
    std::string getHelp() const;
    std::string getCommandHelp(CommandType cmd) const;
    
    // 检查必需的参数
    std::optional<std::string> validate(const ParsedArgs& args) const;
    
private:
    std::map<CommandType, CommandDef> commands_;
    
    // 初始化命令定义
    void initCommands();
    
    // 查找命令类型
    CommandType findCommand(const std::string& name) const;
    
    // 获取参数值（支持长短名称）
    std::optional<std::string> getOptionValue(const std::vector<std::string>& args,
                                               size_t& index,
                                               const std::string& longName,
                                               const std::string& shortName) const;
    
    // 解析布尔选项
    bool hasFlag(const std::vector<std::string>& args,
                 const std::string& longName,
                 const std::string& shortName) const;
};

} // namespace qdrant
