# AGENTS.md - AI 代理使用指南

本文档为 AI 代理（如 OpenClaw）提供 `qdrant-tool` 的使用规范。

## 概述

`qdrant-tool` 是一个命令行工具，为 AI 代理提供长期记忆管理能力。所有命令输出均为 JSON 格式，便于程序解析。

## 基础配置

在使用前，确保环境变量已设置：

```bash
export QDRANT_URL="http://localhost:6333"
export OLLAMA_URL="http://localhost:11434"
export DEFAULT_COLLECTION="memory"
```

## 核心使用场景

### 场景一：保存对话中的重要信息

当用户提及重要信息（如偏好、待办、个人信息）时，使用 `add` 命令保存：

```bash
./qdrant-tool add \
  --collection "user_memory" \
  --content "用户明天下午3点有个会议" \
  --type "todo" \
  --tag "important"
```

### 场景二：检索相关知识

当需要回答关于用户的问题时，使用 `search` 命令检索：

```bash
./qdrant-tool search \
  --collection "user_memory" \
  --query "用户的会议安排" \
  --limit 3 \
  --filter-type "todo"
```

### 场景三：导入文件作为知识库

当用户要求记住文件内容时：

```bash
./qdrant-tool import \
  --file "/path/to/file.txt" \
  --collection "knowledge" \
  --type "document"
```

## 记忆类型建议

| 类型 | 用途 | 示例 |
|------|------|------|
| `preference` | 用户偏好 | 喜欢喝冰美式 |
| `todo` | 待办事项 | 明天开会 |
| `fact` | 客观事实 | 住在上海 |
| `person` | 人物信息 | 妈妈是医生 |
| `event` | 事件记录 | 去年去了日本 |

## 标签使用建议

- `important` - 重要信息
- `private` - 隐私信息
- `work` - 工作相关
- `personal` - 个人生活
- `temp` - 临时信息（可被删除）

## 错误处理

所有命令返回 JSON 格式，检查 `success` 字段：

```bash
response=$(./qdrant-tool add --collection "test" --content "test")

# 解析 JSON 判断是否成功
if echo "$response" | grep -q '"success": true'; then
    echo "操作成功"
else
    echo "操作失败"
    echo "$response"
fi
```

## 注意事项

1. **内容长度**：单次添加的内容建议不超过 4000 字符
2. **集合选择**：
   - `user_memory` - 用户个人记忆
   - `knowledge` - 通用知识
   - `chat_history` - 对话历史
3. **ID 管理**：如果不指定 ID，系统会自动生成 UUID
4. **搜索结果**：默认返回 10 条结果，可通过 `--limit` 调整
