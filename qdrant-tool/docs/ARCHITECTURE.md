# Qdrant Tool 架构文档

## 项目概述

Qdrant Tool 是一个基于 C++17 开发的命令行工具，用于为 AI Agent 提供长期记忆管理能力。它封装了 Qdrant 向量数据库的操作，并通过 Ollama 调用 bge-m3 嵌入模型进行向量化。

## 架构设计

### 分层架构

```
┌─────────────────────────────────────────────────────────┐
│  CLI Layer (命令行接口层)                                 │
│  ├── Argument Parser                                      │
│  ├── Command Router                                       │
│  └── Output Formatter                                     │
├─────────────────────────────────────────────────────────┤
│  Service Layer (业务逻辑层)                               │
│  ├── MemoryService      (记忆管理)                        │
│  ├── CollectionService  (集合管理)                        │
│  └── EvaluationService  (内容评估)                        │
├─────────────────────────────────────────────────────────┤
│  Adapter Layer (适配器层)                                 │
│  ├── QdrantClient     (HTTP Client for Qdrant)            │
│  └── OllamaClient     (HTTP Client for Ollama)            │
├─────────────────────────────────────────────────────────┤
│  Infrastructure Layer (基础设施层)                        │
│  ├── HttpClient       (libcurl wrapper)                   │
│  ├── ConfigManager    (配置管理)                          │
│  └── Result<T>        (错误处理)                          │
└─────────────────────────────────────────────────────────┘
```

### 模块说明

#### 1. CLI Layer

负责命令行参数的解析、命令路由和输出格式化。

**关键类**:
- `ArgumentParser`: 解析命令行参数，支持长短选项、位置参数
- `CommandRouter`: 根据命令类型路由到对应的处理函数
- `ParsedArgs`: 解析后的参数结构

**设计特点**:
- 所有输出统一为 JSON 格式
- 支持命令级帮助信息
- 自动参数验证

#### 2. Service Layer

实现核心业务逻辑，处理记忆管理、集合管理等操作。

**关键类**:
- `MemoryService`: 记忆增删改查、文件导入
- `CollectionService`: 集合管理

**设计特点**:
- 封装向量化细节
- 自动集合创建
- 批量操作支持

#### 3. Adapter Layer

封装外部服务的 HTTP API 调用。

**关键类**:
- `QdrantClient`: Qdrant REST API 封装
- `OllamaClient`: Ollama API 封装

**设计特点**:
- 统一错误处理
- 自动重试机制（可扩展）
- JSON 序列化/反序列化

#### 4. Infrastructure Layer

提供基础设施支持。

**关键类**:
- `HttpClient`: libcurl 封装，支持 GET/POST/PUT/DELETE
- `ConfigManager`: 配置加载（环境变量、配置文件）
- `Result<T>`: 函数式错误处理

**设计特点**:
- 单例模式管理配置
- RAII 管理资源
- 异常安全

### 数据流

#### 添加记忆流程
```
CLI -> MemoryService.add() 
    -> OllamaClient.embed() -> Ollama API
    -> QdrantClient.upsert() -> Qdrant API
    -> JSON Response
```

#### 搜索记忆流程
```
CLI -> MemoryService.search()
    -> OllamaClient.embed() -> Ollama API
    -> QdrantClient.search() -> Qdrant API
    -> JSON Response
```

#### 文件导入流程
```
CLI -> MemoryService.importFile()
    -> FileReader.readAll()
    -> TextChunker.chunk()
    -> OllamaClient.embedBatch() -> Ollama API
    -> QdrantClient.upsert() -> Qdrant API
    -> JSON Response (stats)
```

## 关键技术决策

### 1. 错误处理

采用 `Result<T>` 类型实现函数式错误处理：

```cpp
Result<std::string> add(const AddMemoryRequest& req);

auto result = add(req);
if (result.isError()) {
    // 处理错误
} else {
    // 使用结果
}
```

**优点**:
- 强制错误处理
- 类型安全
- 无需异常传播

### 2. HTTP 客户端

使用 libcurl 进行 HTTP 通信：

**优点**:
- 成熟稳定
- 支持 HTTPS
- 跨平台

### 3. JSON 处理

使用 nlohmann/json 单头文件库：

**优点**:
- 现代 C++ API
- 仅依赖头文件
- 易于集成

### 4. 配置管理

支持多层级配置：
1. 命令行参数（最高优先级）
2. 环境变量
3. 配置文件 (`~/.qdrant-tool/config.json`)
4. 默认值（最低优先级）

## 扩展性设计

### 添加新命令

1. 在 `ArgumentParser` 中添加命令定义
2. 在 `CommandRouter` 中实现执行函数
3. 在 `CommandType` 枚举中添加类型

### 添加新服务

1. 在 `src/service/` 目录创建服务类
2. 在 `CommandRouter` 中注入服务依赖
3. 实现对应的 CLI 命令

### 添加新适配器

1. 在 `src/adapter/` 目录创建客户端类
2. 继承或使用 `HttpClient` 进行 HTTP 通信
3. 实现统一的错误处理模式

## 性能考虑

### 向量化性能

- 支持批量向量化 (`embedBatch`)
- 文件导入时分块批量处理

### 内存管理

- 使用 RAII 管理资源
- 智能指针避免内存泄漏
- 移动语义减少拷贝

### 网络优化

- 连接复用（libcurl 默认）
- 超时控制
- 异步操作（可扩展）

## 安全考虑

### 输入验证

- 命令行参数验证
- JSON 解析错误处理
- 文件路径检查

### 数据安全

- 本地运行，无网络暴露
- 配置文件中可包含敏感信息

## 构建系统

使用 CMake 构建：

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 编译选项

- `-O2`: 优化级别
- `-Wall -Wextra`: 启用警告
- `STATIC_LINK`: 静态链接选项

## 测试策略

### 单元测试

- 各模块独立测试
- Mock 外部依赖

### 集成测试

- 端到端场景测试
- 需要 Qdrant 和 Ollama 服务

## 部署说明

### 环境要求

- Linux 系统
- Qdrant (http://localhost:6333)
- Ollama (http://localhost:11434) + bge-m3 模型

### 安装步骤

```bash
sudo cp build/qdrant-tool /usr/local/bin/
mkdir -p ~/.qdrant-tool
echo '{}' > ~/.qdrant-tool/config.json
```

### 配置示例

```bash
export QDRANT_URL="http://localhost:6333"
export OLLAMA_URL="http://localhost:11434"
export DEFAULT_COLLECTION="memory"
```
