#pragma once

#include <string>
#include <vector>

namespace qdrant {

// 文本分块器类
class TextChunker {
public:
    // 分块选项
    struct Options {
        size_t chunkSize = 500;      // 每个分块的目标大小
        size_t overlap = 50;         // 相邻分块之间的重叠大小
        bool respectBoundaries = true; // 尽量在句子或段落边界处分割
    };
    
    // 构造函数
    explicit TextChunker(const Options& options);
    explicit TextChunker();  // 使用默认选项
    
    // 对文本进行分块
    std::vector<std::string> chunk(const std::string& text) const;
    
    // 设置选项
    void setOptions(const Options& options) { options_ = options; }
    const Options& getOptions() const { return options_; }
    
private:
    Options options_;
    
    // 寻找最佳分割点
    size_t findBestSplitPoint(const std::string& text, size_t targetPos) const;
    
    // 检查字符是否是句子结束符
    bool isSentenceEnd(char c) const;
    
    // 检查字符是否是空白字符
    bool isWhitespace(char c) const;
};

} // namespace qdrant
