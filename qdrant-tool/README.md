# Qdrant Tool

一个基于 C++ 的 Qdrant 向量数据库命令行工具，用于 AI Agent 的长期记忆管理。

## 功能特性

- **记忆管理**：添加、搜索、更新、删除记忆条目
- **向量化**：通过 Ollama 调用 bge-m3 嵌入模型
- **文件导入**：支持文本文件批量导入并自动分块
- **极简设计**：仅保留 content 和 datetime 字段，简化数据结构
- **智能去重**：添加时自动检查相同 content，存在则更新时间戳
- **语义搜索**：基于向量相似度的语义检索

## 系统要求

- Linux 系统
- GCC 12+ 或 Clang
- CMake 3.16+
- libcurl
- Qdrant (本地部署)
- Ollama (本地部署，包含 bge-m3 模型)

## 安装

### 编译安装

```bash
cd qdrant-tool
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### 静态链接编译

```bash
cmake -DSTATIC_LINK=ON ..
make -j$(nproc)
```

## 配置

Qdrant Tool 支持多种配置方式，优先级从高到低：

1. 命令行参数 (`-c/--config`)
2. 环境变量
3. 当前目录配置 (`./qdrant-tool.json`)
4. 用户配置 (`~/.config/qdrant-tool/config.json`)
5. 系统配置 (`/etc/qdrant-tool/config.json`)
6. 内置默认值

### 快速配置

```bash
# 创建用户配置
mkdir -p ~/.config/qdrant-tool
cp config/config.template.json ~/.config/qdrant-tool/config.json

# 编辑配置
vim ~/.config/qdrant-tool/config.json
```

### 环境变量

```bash
export QDRANT_URL="http://localhost:6333"
export OLLAMA_URL="http://localhost:11434"
export EMBEDDING_MODEL="bge-m3"
export DEFAULT_COLLECTION="memory"
```

### 查看当前配置

```bash
./qdrant-tool --show-config
```

详细配置说明见 [docs/CONFIGURATION.md](docs/CONFIGURATION.md)

## 使用说明

### 添加记忆

```bash
./qdrant-tool add \
  --collection "user_memory" \
  --content "用户喜欢喝冰美式"
```

如果 content 已存在，会自动更新 datetime 时间戳。

### 搜索记忆

```bash
./qdrant-tool search \
  --collection "user_memory" \
  --query "用户喜欢喝什么" \
  --limit 5 \
  --min-score 0.7
```

### 导入文件

```bash
./qdrant-tool import \
  --file "/path/to/document.txt" \
  --collection "documents" \
  --chunk-size 500 \
  --chunk-overlap 50
```

导入的数据仅包含 `content` 字段。

### 删除记忆

```bash
# 按 ID 删除
./qdrant-tool delete --collection "user_memory" --id "afc8cefd-4806-40a0-8abd-b4221a7ad6b4"

# 按内容删除（内容必须完全匹配）
./qdrant-tool delete --collection "user_memory" --content "用户喜欢喝冰美式"
```

### 集合管理

```bash
# 创建集合
./qdrant-tool collection create --name "my_collection" --dimension 1024

# 列出集合
./qdrant-tool collection list

# 查看集合信息
./qdrant-tool collection info --name "my_collection"

# 删除集合
./qdrant-tool collection delete --name "my_collection"
```

## 命令参考

| 命令 | 描述 |
|------|------|
| `add` | 添加记忆条目 |
| `search` | 语义搜索 |
| `delete` | 删除记忆 |
| `update` | 更新记忆 |
| `import` | 导入文件 |
| `collection create` | 创建集合 |
| `collection list` | 列出集合 |
| `collection info` | 集合信息 |
| `collection delete` | 删除集合 |
| `health` | 健康检查 |
| `eval` | 内容评估 |

## 输出格式

所有命令输出均为 JSON 格式：

```json
{
  "success": true,
  "id": "memory_001"
}
```

错误输出：

```json
{
  "success": false,
  "error": {
    "code": "E001",
    "message": "Parameter error"
  }
}
```

## 许可证

MIT License
