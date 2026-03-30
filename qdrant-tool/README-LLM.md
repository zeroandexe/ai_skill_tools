# qdrant-tool SKILL

Qdrant 向量数据库 CLI 工具，用于 AI Agent 长期记忆管理。

## 核心特性

- **极简数据模型**：仅保留 `content` 和 `datetime` 字段
- **智能去重**：添加相同 content 时自动更新时间戳
- **语义搜索**：基于 bge-m3 向量模型的语义检索
- **文件导入**：自动分块、批量向量化

---

## 命令详解

### add - 添加/更新记忆

**功能**：添加新记忆。如果 content 已存在，则更新该记录的 datetime。

```bash
qdrant-tool add --collection <name> --content <text> [--id <uuid>]
```

**参数**：
- `--collection, -c` (required): 集合名称
- `--content` (required): 内容文本
- `--id` (optional): 自定义 ID，默认自动生成 UUID

**返回示例**：
```json
{
  "success": true,
  "id": "afc8cefd-4806-40a0-8abd-b4221a7ad6b4"
}
```

**错误返回**：
```json
{
  "success": false,
  "error": {
    "code": "E001",
    "message": "Content is required"
  }
}
```

---

### search - 语义搜索

**功能**：基于语义相似度搜索记忆。

```bash
qdrant-tool search --collection <name> --query <text> [--limit N] [--min-score 0.0-1.0]
```

**参数**：
- `--collection, -c` (required): 集合名称
- `--query, -q` (required): 搜索查询文本
- `--limit, -l` (optional): 返回结果数量，默认 10
- `--min-score` (optional): 最小相似度阈值 (0-1)，默认 0.0

**返回示例**：
```json
{
  "success": true,
  "count": 2,
  "results": [
    {
      "id": "afc8cefd-4806-40a0-8abd-b4221a7ad6b4",
      "score": 0.850892,
      "content": "用户喜欢喝冰美式",
      "datetime": "2026/03/19 21:15:32"
    },
    {
      "id": "b7d2a1f3-9e5c-4b8a-9d1f-3c6e8a2b5d4e",
      "score": 0.723456,
      "content": "用户每天早上去咖啡店",
      "datetime": "2026/03/18 09:30:00"
    }
  ]
}
```
---

### delete - 删除记忆

**功能**：删除指定 ID 或内容的记忆。

```bash
# 按 ID 删除
qdrant-tool delete --collection <name> --id <uuid>

# 按内容删除（内容必须完全匹配）
qdrant-tool delete --collection <name> --content "要删除的内容"
```

**参数**：
- `--collection, -c` (required): 集合名称
- `--id` (optional): 记忆 ID
- `--content` (optional): 要删除的完整内容（与存储内容完全匹配）

**注意**：`--id` 和 `--content` 必须提供其中一个。

**返回示例 - 按 ID 删除成功**：
```json
{
  "success": true,
  "message": "Memory deleted successfully"
}
```

**返回示例 - 按内容删除成功**：
```json
{
  "success": true,
  "message": "Memory deleted successfully"
}
```

**返回示例 - 内容未找到**：
```json
{
  "success": true,
  "message": "No matching memory found"
}
```

---

### update - 更新记忆

**功能**：更新指定 ID 的记忆内容。

```bash
qdrant-tool update --collection <name> --id <uuid> [--content <text>]
```

**参数**：
- `--collection, -c` (required): 集合名称
- `--id` (required): 记忆 ID
- `--content` (optional): 新内容

**返回示例**：
```json
{
  "success": true,
  "message": "Memory updated successfully"
}
```

---

### import - 文件导入

**功能**：导入文本文件，自动分块并向量化存储。

```bash
qdrant-tool import --file <path> [--collection <name>] [--chunk-size 500] [--chunk-overlap 50]
```

**参数**：
- `--file, -f` (required): 文件路径
- `--collection, -c` (optional): 集合名称，默认使用文件名（不含扩展名）
- `--chunk-size` (optional): 分块大小（字符数），默认 500
- `--chunk-overlap` (optional): 分块重叠大小，默认 50

**返回示例**：
```json
{
  "success": true,
  "total_chunks": 12,
  "success_count": 12,
  "failed_count": 0
}
```

**导入数据格式**：
每个 chunk 仅包含以下字段：
```json
{
  "content": "分块文本内容",
  "datetime": "2026/03/19 21:15:32"
}
```

---

### collection - 集合管理

**创建集合**：
```bash
qdrant-tool collection create --name <name> [--dimension 1024] [--distance Cosine]
```

**列出所有集合**：
```bash
qdrant-tool collection list
```
返回：
```json
{
  "success": true,
  "collections": ["my_base", "documents", "memory"]
}
```

**查看集合信息**：
```bash
qdrant-tool collection info --name <name>
```
返回：
```json
{
  "success": true,
  "name": "my_base",
  "dimension": 1024,
  "points_count": 150,
  "distance": "Cosine"
}
```

**删除集合**：
```bash
qdrant-tool collection delete --name <name>
```

---

## 数据存储格式

### 完整数据结构

```json
{
  "id": "afc8cefd-4806-40a0-8abd-b4221a7ad6b4",
  "vector": [0.023, -0.156, ...],  // 1024 维向量
  "payload": {
    "content": "文本内容",
    "datetime": "2026/03/19 21:15:32"
  }
}
```

### 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | UUID v4 格式 |
| `vector` | float[] | bge-m3 生成的 1024 维向量 |
| `payload.content` | string | 文本内容 |
| `payload.datetime` | string | 插入/更新时间，格式 YYYY/MM/DD HH:MM:SS |

---

## 环境变量

| 变量名 | 默认值 | 说明 |
|--------|--------|------|
| `QDRANT_URL` | http://localhost:6333 | Qdrant 服务地址 |
| `OLLAMA_URL` | http://localhost:11434 | Ollama 服务地址 |
| `EMBEDDING_MODEL` | bge-m3 | 嵌入模型名称 |
| `DEFAULT_COLLECTION` | memory | 默认集合名称 |

---

## 配置优先级

从高到低：
1. 命令行参数
2. 环境变量
3. `./qdrant-tool.json`（当前目录）
4. `~/.config/qdrant-tool/config.json`（用户配置）
5. `/etc/qdrant-tool/config.json`（系统配置）
6. 内置默认值

---

## Skill 配置示例

### OpenClaw / Claw 格式

```yaml
name: memory_manager
description: 长期记忆管理工具
tools:
  save_memory:
    description: 保存记忆，相同内容会自动更新时间戳
    command: "qdrant-tool add --collection {{collection|default('memory')}} --content '{{content}}'"
    
  recall_memory:
    description: 语义搜索记忆
    command: "qdrant-tool search --collection {{collection|default('memory')}} --query '{{query}}' --limit {{limit|default(5)}} --min-score {{min_score|default(0.6)}}"
    
  forget_memory:
    description: 删除指定记忆
    command: "qdrant-tool delete --collection {{collection|default('memory')}} --id {{id}}"
    
  import_document:
    description: 导入文档并自动分块
    command: "qdrant-tool import --file '{{file_path}}' --collection {{collection}} --chunk-size {{chunk_size|default(500)}}"
```

---

## 典型使用场景

### 场景 1：保存用户偏好
```bash
# 第一次保存
qdrant-tool add --collection user_prefs --content "用户喜欢喝冰美式"
# 返回: {"success": true, "id": "xxx-1"}

# 再次保存相同内容（自动更新时间戳）
qdrant-tool add --collection user_prefs --content "用户喜欢喝冰美式"
# 返回: {"success": true, "id": "xxx-1"}  （相同 ID，datetime 已更新）
```

### 场景 2：语义搜索
```bash
qdrant-tool search --collection user_prefs --query "用户喜欢什么咖啡" --limit 3 --min-score 0.5
```

### 场景 3：导入知识库
```bash
qdrant-tool import --file ./knowledge_base.txt --collection knowledge --chunk-size 800 --chunk-overlap 100
```

---

## 错误码说明

| 错误码 | 说明 |
|--------|------|
| `E001` | 参数错误 |
| `E002` | 集合不存在 |
| `E003` | 点（记忆）不存在 |
| `E100` | Qdrant 连接失败 |
| `E200` | Ollama 连接失败 |
| `E999` | 内部错误 |

---

## 技术规格

- **向量维度**：1024 (bge-m3)
- **向量距离**：Cosine
- **ID 格式**：UUID v4
- **时间格式**：YYYY/MM/DD HH:MM:SS
- **输出格式**：JSON
- **批量大小**：10（import 时批量插入）
- **去重相似度阈值**：0.99
