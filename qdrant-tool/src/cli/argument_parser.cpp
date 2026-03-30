#include "argument_parser.h"
#include <iostream>
#include <algorithm>

namespace qdrant {

ArgumentParser::ArgumentParser() {
    initCommands();
}

void ArgumentParser::initCommands() {
    // ADD 命令
    commands_[CommandType::ADD] = {
        "add",
        "Add or update a memory entry (updates datetime if content exists)",
        {
            {"collection", "c", "Collection name", true, true, ""},
            {"content", "", "Content to store", true, true, ""},
            {"id", "", "Optional ID (auto-generated if not provided)", false, true, ""}
        }
    };
    
    // SEARCH 命令
    commands_[CommandType::SEARCH] = {
        "search",
        "Search memories by semantic similarity",
        {
            {"collection", "c", "Collection name", true, true, ""},
            {"query", "q", "Search query", true, true, ""},
            {"limit", "l", "Maximum number of results", false, true, "10"},
            {"min-score", "", "Minimum similarity score (0-1)", false, true, "0.0"}
        }
    };
    
    // DELETE 命令
    commands_[CommandType::DELETE] = {
        "delete",
        "Delete memory entries by ID or content",
        {
            {"collection", "c", "Collection name", true, true, ""},
            {"id", "", "ID of the memory to delete", false, true, ""},
            {"content", "", "Exact content to delete (must match completely)", false, true, ""}
        }
    };
    
    // UPDATE 命令
    commands_[CommandType::UPDATE] = {
        "update",
        "Update an existing memory",
        {
            {"collection", "c", "Collection name", true, true, ""},
            {"id", "", "ID of the memory to update", true, true, ""},
            {"content", "", "New content", false, true, ""}
        }
    };
    
    // IMPORT 命令
    commands_[CommandType::IMPORT] = {
        "import",
        "Import memories from a file",
        {
            {"file", "f", "File path to import", true, true, ""},
            {"collection", "c", "Collection name (default: file name)", false, true, ""},
            {"chunk-size", "", "Size of each chunk", false, true, "500"},
            {"chunk-overlap", "", "Overlap between chunks", false, true, "50"}
        }
    };
    
    // COLLECTION 命令
    commands_[CommandType::COLLECTION_CREATE] = {
        "collection create",
        "Create a new collection",
        {
            {"name", "n", "Collection name", true, true, ""},
            {"dimension", "d", "Vector dimension", false, true, "1024"},
            {"distance", "", "Distance metric (Cosine/Euclid/Dot)", false, true, "Cosine"}
        }
    };
    
    commands_[CommandType::COLLECTION_LIST] = {
        "collection list",
        "List all collections",
        {}
    };
    
    commands_[CommandType::COLLECTION_INFO] = {
        "collection info",
        "Show collection information",
        {
            {"name", "n", "Collection name", true, true, ""}
        }
    };
    
    commands_[CommandType::COLLECTION_DELETE] = {
        "collection delete",
        "Delete a collection",
        {
            {"name", "n", "Collection name", true, true, ""}
        }
    };
    
    // HEALTH 命令
    commands_[CommandType::HEALTH] = {
        "health",
        "Check service health",
        {}
    };
    
    // EVAL 命令
    commands_[CommandType::EVAL] = {
        "eval",
        "Evaluate content importance",
        {
            {"content", "c", "Content to evaluate", true, true, ""}
        }
    };
}

ParsedArgs ArgumentParser::parse(int argc, char** argv) {
    ParsedArgs result;
    
    if (argc < 2) {
        result.help = true;
        return result;
    }
    
    std::vector<std::string> args(argv + 1, argv + argc);
    
    // 检查全局选项
    if (args[0] == "-h" || args[0] == "--help") {
        result.help = true;
        return result;
    }
    if (args[0] == "-v" || args[0] == "--version") {
        result.version = true;
        return result;
    }
    
    // 解析命令
    std::string cmdName = args[0];
    result.command = findCommand(cmdName);
    
    // 处理 collection 子命令
    if (cmdName == "collection" && args.size() > 1) {
        std::string subCmd = args[1];
        result.subcommand = subCmd;
        result.command = findCommand("collection " + subCmd);
        
        // 移除前两个参数
        args.erase(args.begin(), args.begin() + 2);
    } else {
        // 移除第一个参数（命令）
        args.erase(args.begin());
    }
    
    // 检查帮助选项
    for (const auto& arg : args) {
        if (arg == "-h" || arg == "--help") {
            result.help = true;
            return result;
        }
    }
    
    // 解析选项
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg == "--") {
            // 剩余的都是位置参数
            for (size_t j = i + 1; j < args.size(); ++j) {
                result.positional.push_back(args[j]);
            }
            break;
        }
        
        if (arg.substr(0, 2) == "--") {
            // 长选项
            std::string optName = arg.substr(2);
            size_t eqPos = optName.find('=');
            
            if (eqPos != std::string::npos) {
                // --name=value
                std::string name = optName.substr(0, eqPos);
                std::string value = optName.substr(eqPos + 1);
                result.options[name] = value;
            } else if (i + 1 < args.size() && args[i + 1][0] != '-') {
                // --name value
                result.options[optName] = args[++i];
            } else {
                // --name (flag)
                result.options[optName] = "true";
            }
        } else if (arg.substr(0, 1) == "-" && arg.length() > 1) {
            // 短选项
            std::string optName = arg.substr(1);
            
            if (optName.length() == 1) {
                // 单字符选项
                if (i + 1 < args.size() && args[i + 1][0] != '-') {
                    result.options[std::string(1, optName[0])] = args[++i];
                } else {
                    result.options[std::string(1, optName[0])] = "true";
                }
            }
        } else {
            // 位置参数
            result.positional.push_back(arg);
        }
    }
    
    return result;
}

std::string ArgumentParser::getHelp() const {
    std::string help = "qdrant-tool - Qdrant Vector Database CLI Tool\n\n";
    help += "Usage: qdrant-tool <command> [options]\n\n";
    help += "Commands:\n";
    
    for (const auto& [type, def] : commands_) {
        if (type != CommandType::UNKNOWN) {
            help += "  " + def.name;
            // 对齐
            help += std::string(25 - def.name.length(), ' ');
            help += def.description + "\n";
        }
    }
    
    help += "\nGlobal Options:\n";
    help += "  -h, --help     Show this help message\n";
    help += "  -v, --version  Show version information\n";
    help += "\nEnvironment Variables:\n";
    help += "  QDRANT_URL         Qdrant server URL (default: http://localhost:6333)\n";
    help += "  OLLAMA_URL         Ollama server URL (default: http://localhost:11434)\n";
    help += "  EMBEDDING_MODEL    Embedding model name (default: bge-m3)\n";
    help += "  DEFAULT_COLLECTION Default collection name (default: memory)\n";
    
    help += "\nUse 'qdrant-tool <command> --help' for command-specific help.\n";
    
    return help;
}

std::string ArgumentParser::getCommandHelp(CommandType cmd) const {
    auto it = commands_.find(cmd);
    if (it == commands_.end()) {
        return "Unknown command\n";
    }
    
    const auto& def = it->second;
    std::string help = "Usage: qdrant-tool " + def.name;
    
    for (const auto& arg : def.args) {
        if (arg.required) {
            help += " --" + arg.name + " <value>";
        } else {
            help += " [--" + arg.name + " <value>]";
        }
    }
    
    help += "\n\n" + def.description + "\n\n";
    
    if (!def.args.empty()) {
        help += "Options:\n";
        for (const auto& arg : def.args) {
            help += "  --" + arg.name;
            if (!arg.shortName.empty()) {
                help += ", -" + arg.shortName;
            }
            help += std::string(20 - arg.name.length() - arg.shortName.length(), ' ');
            help += arg.description;
            if (!arg.defaultValue.empty()) {
                help += " (default: " + arg.defaultValue + ")";
            }
            if (arg.required) {
                help += " [required]";
            }
            help += "\n";
        }
    }
    
    return help;
}

std::optional<std::string> ArgumentParser::validate(const ParsedArgs& args) const {
    if (args.command == CommandType::UNKNOWN) {
        return "Unknown command";
    }
    
    if (args.help || args.version) {
        return std::nullopt;
    }
    
    auto it = commands_.find(args.command);
    if (it == commands_.end()) {
        return "Unknown command";
    }
    
    const auto& def = it->second;
    
    // 检查必需参数
    for (const auto& arg : def.args) {
        if (arg.required) {
            if (args.options.find(arg.name) == args.options.end() &&
                args.options.find(arg.shortName) == args.options.end()) {
                return "Missing required option: --" + arg.name;
            }
        }
    }
    
    return std::nullopt;
}

CommandType ArgumentParser::findCommand(const std::string& name) const {
    for (const auto& [type, def] : commands_) {
        if (def.name == name) {
            return type;
        }
    }
    
    if (name == "collection") {
        return CommandType::COLLECTION_LIST;
    }
    
    return CommandType::UNKNOWN;
}

std::optional<std::string> ArgumentParser::getOptionValue(
    const std::vector<std::string>& args,
    size_t& index,
    const std::string& longName,
    const std::string& shortName) const {
    
    const std::string& arg = args[index];
    
    if (arg == "--" + longName || (!shortName.empty() && arg == "-" + shortName)) {
        if (index + 1 < args.size()) {
            return args[++index];
        }
    }
    
    return std::nullopt;
}

bool ArgumentParser::hasFlag(const std::vector<std::string>& args,
                              const std::string& longName,
                              const std::string& shortName) const {
    for (const auto& arg : args) {
        if (arg == "--" + longName || (!shortName.empty() && arg == "-" + shortName)) {
            return true;
        }
    }
    return false;
}

} // namespace qdrant
