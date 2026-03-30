#include "text_chunker.h"
#include <algorithm>

namespace qdrant {

TextChunker::TextChunker(const Options& options) : options_(options) {}

TextChunker::TextChunker() : options_(Options{}) {}

std::vector<std::string> TextChunker::chunk(const std::string& text) const {
    std::vector<std::string> chunks;
    
    if (text.empty()) {
        return chunks;
    }
    
    if (text.length() <= options_.chunkSize) {
        chunks.push_back(text);
        return chunks;
    }
    
    size_t start = 0;
    while (start < text.length()) {
        size_t end = std::min(start + options_.chunkSize, text.length());
        
        // 如果不是最后一块且需要尊重边界
        if (end < text.length() && options_.respectBoundaries) {
            end = findBestSplitPoint(text, end);
        }
        
        // 提取分块
        chunks.push_back(text.substr(start, end - start));
        
        // 计算下一个起始位置（考虑重叠）
        if (end >= text.length()) {
            break;
        }
        
        start = end;
        if (options_.overlap > 0 && start > options_.overlap) {
            start -= options_.overlap;
        }
    }
    
    return chunks;
}

size_t TextChunker::findBestSplitPoint(const std::string& text, size_t targetPos) const {
    // 向前搜索句子边界
    size_t searchStart = targetPos;
    size_t searchEnd = std::min(targetPos + options_.chunkSize / 4, text.length());
    
    // 优先寻找段落边界 (\n\n)
    for (size_t i = searchStart; i < searchEnd - 1 && i + 1 < text.length(); ++i) {
        if (text[i] == '\n' && text[i + 1] == '\n') {
            return i + 2;
        }
    }
    
    // 其次寻找句子边界 (.!? 后跟空格或换行)
    for (size_t i = searchStart; i < searchEnd && i < text.length(); ++i) {
        if (isSentenceEnd(text[i])) {
            size_t j = i + 1;
            while (j < text.length() && isWhitespace(text[j])) {
                ++j;
            }
            return j;
        }
    }
    
    // 最后寻找单词边界 (空格)
    for (size_t i = searchStart; i < searchEnd && i < text.length(); ++i) {
        if (isWhitespace(text[i])) {
            return i + 1;
        }
    }
    
    // 如果没有找到合适的边界，就在目标位置分割
    return targetPos;
}

bool TextChunker::isSentenceEnd(char c) const {
    // 只检查 ASCII 字符，中文字符通过其他方式处理
    return c == '.' || c == '!' || c == '?';
}

bool TextChunker::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

} // namespace qdrant
