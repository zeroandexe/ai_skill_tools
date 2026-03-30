# Qdrant Tool 配置系统

## 概述

Qdrant Tool 支持灵活的配置系统，配置优先级从高到低：

1. **命令行参数** (`-c/--config`)
2. **环境变量** (`QDRANT_TOOL_CONFIG` 等)
3. **当前目录配置** (`./qdrant-tool.json`)
4. **用户配置** (`~/.config/qdrant-tool/config.json`)
5. **系统配置** (`/etc/qdrant-tool/config.json`)
6. **安装目录默认配置** (`<安装目录>/config/default.json`)
7. **内置默认值** (最低优先级)

## 配置文件搜索路径

```
QDRANT_TOOL_CONFIG (环境变量指定)
    ↓
./qdrant-tool.json (当前目录)
    ↓
~/.config/qdrant-tool/config.json (用户配置)
    ↓
/etc/qdrant-tool/config.json (系统配置)
    ↓
<安装目录>/config/default.json (默认配置)
    ↓
内置默认值
```

## 配置文件格式

JSON 格式，示例：

```json
{
  "qdrantUrl": "http://localhost:6333",
  "ollamaUrl": "http://localhost:11434",
  "embeddingModel": "bge-m3",
  "embeddingDimension": 1024,
  "defaultCollection": "memory",
  "connectTimeout": 5000,
  "requestTimeout": 30000,
  "enableLog": false,
  "logLevel": "info"
}
```

## 配置项说明

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| `qdrantUrl` | Qdrant 服务地址 | `http://localhost:6333` |
| `ollamaUrl` | Ollama 服务地址 | `http://localhost:11434` |
| `embeddingModel` | 嵌入模型名称 | `bge-m3` |
| `embeddingDimension` | 向量维度 | `1024` |
| `defaultCollection` | 默认集合名 | `memory` |
| `connectTimeout` | 连接超时(毫秒) | `5000` |
| `requestTimeout` | 请求超时(毫秒) | `30000` |
| `enableLog` | 启用日志 | `false` |
| `logLevel` | 日志级别 | `info` |

## 使用方式

### 方式 1：使用环境变量

```bash
export QDRANT_URL="http://custom-server:6333"
export OLLAMA_URL="http://custom-ollama:11434"
export EMBEDDING_MODEL="bge-m3"
export DEFAULT_COLLECTION="my_memory"

./qdrant-tool add --content "test"
```

### 方式 2：使用用户配置文件

```bash
# 创建配置目录和文件
mkdir -p ~/.config/qdrant-tool
cat > ~/.config/qdrant-tool/config.json << 'EOF'
{
  "qdrantUrl": "http://custom-server:6333",
  "ollamaUrl": "http://custom-ollama:11434",
  "defaultCollection": "my_memory"
}
EOF

# 运行命令（自动加载配置）
./qdrant-tool add --content "test"
```

### 方式 3：使用指定配置文件

```bash
# 创建自定义配置文件
cat > /path/to/my-config.json << 'EOF'
{
  "qdrantUrl": "http://prod-server:6333",
  "defaultCollection": "prod_memory"
}
EOF

# 使用 -c 或 --config 指定
./qdrant-tool -c /path/to/my-config.json add --content "test"
```

### 方式 4：使用当前目录配置

```bash
cd /my/project
cat > qdrant-tool.json << 'EOF'
{
  "defaultCollection": "project_memory"
}
EOF

# 自动加载当前目录配置
./qdrant-tool add --content "test"
```

## 查看当前配置

使用 `--show-config` 查看实际生效的配置：

```bash
./qdrant-tool --show-config
```

输出示例：

```json
{
  "_sources": [
    "built-in defaults",
    "/home/user/.config/qdrant-tool/config.json"
  ],
  "qdrantUrl": "http://custom-server:6333",
  "ollamaUrl": "http://localhost:11434",
  "embeddingModel": "bge-m3",
  "embeddingDimension": 1024,
  "defaultCollection": "my_memory",
  "connectTimeout": 5000,
  "requestTimeout": 30000,
  "enableLog": false,
  "logLevel": "info"
}
```

其中 `_sources` 显示了配置加载的来源。

## 部署建议

### 开发环境

使用用户配置文件：

```bash
mkdir -p ~/.config/qdrant-tool
cp config/config.template.json ~/.config/qdrant-tool/config.json
# 编辑 ~/.config/qdrant-tool/config.json 修改配置
```

### 生产环境

使用系统配置或环境变量：

```bash
# 系统管理员配置
sudo mkdir -p /etc/qdrant-tool
sudo cp config/default.json /etc/qdrant-tool/config.json
sudo vim /etc/qdrant-tool/config.json

# 或使用环境变量
export QDRANT_URL="http://prod-qdrant:6333"
export OLLAMA_URL="http://prod-ollama:11434"
```

### Docker 环境

```dockerfile
# 复制配置到镜像
COPY config/production.json /etc/qdrant-tool/config.json

# 或使用环境变量
ENV QDRANT_URL=http://qdrant:6333
ENV OLLAMA_URL=http://ollama:11434
```

## 配置合并规则

后面的配置会覆盖前面的配置：

```bash
# 示例：配置合并过程
# 1. 加载内置默认值
# 2. 加载安装目录 config/default.json（覆盖部分值）
# 3. 加载 /etc/qdrant-tool/config.json（覆盖部分值）
# 4. 加载 ~/.config/qdrant-tool/config.json（覆盖部分值）
# 5. 环境变量 QDRANT_URL 覆盖 qdrantUrl
```

## 故障排查

### 检查配置是否加载

```bash
./qdrant-tool --show-config
```

### 检查配置文件路径

```bash
# 查看搜索路径
ls -la /etc/qdrant-tool/config.json
ls -la ~/.config/qdrant-tool/config.json
ls -la ./qdrant-tool.json
```

### 验证 JSON 格式

```bash
# 使用 jq 验证
jq . ~/.config/qdrant-tool/config.json

# 或使用 Python
python3 -m json.tool ~/.config/qdrant-tool/config.json
```

## 配置文件模板

项目提供 `config/config.template.json` 作为模板：

```bash
cp config/config.template.json ~/.config/qdrant-tool/config.json
vim ~/.config/qdrant-tool/config.json
```

## 注意事项

1. **不需要重新编译**：修改配置文件后立即可生效
2. **部分配置**：配置文件可以只包含部分配置项，缺失的会使用默认值
3. **类型安全**：配置项必须符合 JSON 类型（字符串、数字、布尔值）
4. **注释**：JSON 不支持注释，可以使用 `_comment` 字段作为说明
