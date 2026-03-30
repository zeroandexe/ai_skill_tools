# Qdrant Tool 测试文档

## 测试结构

```
tests/
├── README.md                # 本文档
├── test_framework.sh        # Bash 测试框架
├── unit/                    # 单元测试
│   └── test_uuid.sh         # UUID 工具测试示例
├── integration/             # 集成测试
│   ├── run_tests.sh         # 完整测试套件
│   ├── quick_test.sh        # 快速功能验证
│   └── results/             # 测试结果输出
└── fixtures/                # 测试数据文件
```

## 测试类型

### 1. 快速测试 (quick_test.sh)

用于快速验证基本功能是否正常工作。

```bash
cd qdrant-tool
./tests/integration/quick_test.sh
```

测试项目：
- 工具可执行
- 版本输出
- 集合创建/列出/信息/删除
- 记忆添加/搜索

### 2. 完整集成测试 (run_tests.sh)

全面的功能测试，覆盖所有命令和场景。

```bash
cd qdrant-tool
./tests/integration/run_tests.sh
```

测试套件：
1. **Version and Help** - 版本和帮助信息
2. **Collection Management** - 集合管理
3. **Memory Add** - 添加记忆
4. **Memory Search** - 搜索记忆
5. **Memory Update** - 更新记忆
6. **Memory Delete** - 删除记忆
7. **File Import** - 文件导入
8. **Error Handling** - 错误处理
9. **Performance** - 性能测试
10. **Special Characters** - 特殊字符处理

### 3. 单元测试示例

目前提供 Bash 风格的单元测试示例。实际 C++ 单元测试需要集成 Catch2 等框架。

```bash
cd qdrant-tool/tests
source test_framework.sh
# 运行单元测试
bash unit/test_uuid.sh
```

## 环境要求

- Qdrant 服务运行在 http://localhost:6333
- Ollama 服务运行在 http://localhost:11434
- bge-m3 模型已加载到 Ollama

## 测试配置

通过环境变量配置测试：

```bash
export QDRANT_TOOL=/path/to/qdrant-tool
export QDRANT_URL=http://localhost:6333
export OLLAMA_URL=http://localhost:11434
```

## 测试结果

测试结果保存在 `tests/integration/results/` 目录：
- `test_N.log` - 单个测试的详细输出
- `report_YYYYMMDD_HHMMSS.txt` - 测试报告

## 测试报告示例

```
Qdrant Tool Test Report
=======================
Date: Thu Mar 19 17:37:12 CST 2026
Tool: ./build/qdrant-tool

Summary:
  Total Tests:  32
  Passed:       32
  Failed:       0
  Skipped:      0
  Pass Rate:    100.0%
```

## 添加新测试

### 添加到集成测试

在 `run_tests.sh` 中添加新的测试函数：

```bash
test_new_feature() {
    echo -e "${YELLOW}Test Suite X: New Feature${NC}"
    
    run_test "Test description" \
        "${QDRANT_TOOL} command --arg value" \
        "success" \
        "expected_pattern"
    
    echo ""
}
```

然后在 `main()` 中调用 `test_new_feature`。

### 使用测试框架

```bash
source test_framework.sh

test_my_feature() {
    assert_equals "expected" "actual" "Test description"
    assert_true "[[ -f file.txt ]]" "File should exist"
}

test_my_feature
print_test_report
```

## 注意事项

1. 测试会创建临时集合，测试完成后自动清理
2. 性能测试的阈值可能需要根据环境调整
3. 中文测试需要终端支持 UTF-8
4. 如果服务未启动，测试会失败

## CI/CD 集成

测试脚本返回非零退出码表示失败，可用于 CI：

```yaml
# .github/workflows/test.yml 示例
test:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v2
    - name: Build
      run: |
        mkdir build && cd build
        cmake .. && make
    - name: Start services
      run: |
        docker run -d -p 6333:6333 qdrant/qdrant
        # 启动 Ollama...
    - name: Run tests
      run: ./tests/integration/run_tests.sh
```
